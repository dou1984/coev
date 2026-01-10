#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <random>
#include <cstring>
#include <coev/coev.h>
#include "undefined.h"
#include "access_token.h"
#include "broker.h"
#include "kerberos_client.h"
#include "scram_client.h"
#include "response_header.h"

int8_t getHeaderLength(int16_t headerVersion)
{
    if (headerVersion < 1)
    {
        return 8;
    }
    else
    {
        return 9;
    }
}

Broker::Broker(const std::string &addr) : m_id(-1), m_addr(addr), m_opened(false), m_correlation_id(0), m_client_session_reauthentication_time(0)
{
}
Broker::Broker(int id, const std::string &addr) : m_id(id), m_addr(addr), m_opened(false), m_correlation_id(0), m_client_session_reauthentication_time(0)
{
}
coev::awaitable<int> Broker::Open(std::shared_ptr<Config> conf)
{
    bool expected = false;
    if (!m_opened.compare_exchange_strong(expected, true))
    {
        co_return 1;
    }
    if (!conf)
    {
        conf = std::make_shared<Config>();
    }
    int32_t err = conf->Validate();
    if (err)
    {
        co_return err;
    }
    std::lock_guard<std::mutex> lock(m_lock);
    if (!m_metric_registry)
    {
        m_metric_registry = metrics::NewRegistry();
    }
    m_conf = conf;
    m_task << [this]() -> coev::awaitable<int>
    {
        std::lock_guard<std::mutex> lock(m_lock);

        auto [host, port] = net::SplitHostPort(m_addr);
        auto fd = co_await m_conn.connect(host.data(), port);
        if (fd == INVALID)
        {
            co_return ErrNotConnected;
        }

        m_incoming_byte_rate = metrics::GetOrRegisterMeter("incoming-byte-rate", m_metric_registry);

        m_request_rate = metrics::GetOrRegisterMeter("request-rate", m_metric_registry);

        m_fetch_rate = metrics::GetOrRegisterMeter("consumer-fetch-rate", m_metric_registry);

        m_request_size = metrics::GetOrRegisterHistogram("request-size", m_metric_registry);

        m_request_latency = metrics::GetOrRegisterHistogram("request-latency-in-ms", m_metric_registry);

        m_outgoing_byte_rate = metrics::GetOrRegisterMeter("outgoing-byte-rate", m_metric_registry);

        m_response_rate = metrics::GetOrRegisterMeter("response-rate", m_metric_registry);

        m_response_size = metrics::GetOrRegisterHistogram("response-size", m_metric_registry);

        m_requests_in_flight = metrics::GetOrRegisterCounter("requests-in-flight", m_metric_registry);

        if (m_id >= 0)
        {
            RegisterMetrics();
        }
        int err = ErrNoError;
        if (m_conf->ApiVersionsRequest)
        {
            std::shared_ptr<ApiVersionsResponse> apiVersionsResponse;
            err = co_await SendAndReceiveApiVersions(3, apiVersionsResponse);
            if (err)
            {
                int16_t maxVersion = 0;
                if (apiVersionsResponse)
                {
                    for (auto &k : apiVersionsResponse->m_api_keys)
                    {
                        if (k.m_api_key == apiKeyApiVersions)
                        {
                            maxVersion = k.m_max_version;
                            break;
                        }
                    }
                }
                err = co_await SendAndReceiveApiVersions(maxVersion, apiVersionsResponse);
                if (err)
                {
                    co_return err;
                }
            }
            if (apiVersionsResponse)
            {

                m_broker_api_versions.clear();
                for (auto &key : apiVersionsResponse->m_api_keys)
                {
                    m_broker_api_versions.emplace(key.m_api_key, ApiVersionRange(key.m_min_version, key.m_max_version));
                }
            }
        }

        bool useSaslV0 = m_conf->Net.SASL.Version == 0;
        if (m_conf->Net.SASL.Enable && useSaslV0)
        {

            err = co_await AuthenticateViaSASLv0();
            if (err)
            {
                m_conn.close();
                m_opened.store(false);
                co_return ErrPermitForbidden;
            }
        }

        m_task << SesponseReceiver();
        if (m_conf->Net.SASL.Enable && !useSaslV0)
        {
            err = co_await AuthenticateViaSASLv1();
            if (err)
            {
                co_await m_done.get();
                int32_t closeErr = m_conn.close();
                m_opened.store(false);
                co_return err;
            }
        }
    }();

    co_return 0;
}

int Broker::ResponseSize()
{
    std::lock_guard<std::mutex> lock(m_lock);
    return m_responses.size();
}

bool Broker::Connected()
{
    std::lock_guard<std::mutex> lock(m_lock);
    return m_conn;
}

int Broker::TLSConnectionState()
{
    std::lock_guard<std::mutex> lock(m_lock);
    tls::ConnectionState state;
    if (!m_conn)
    {
        return state;
    }

    // if (auto bconn = (buf::Conn *)(conn))
    // {
    //     return bconn->ConnectionState();
    // }
    // if (auto tc = (tls::Conn *)(conn))
    // {
    //     return tc->ConnectionState();
    // }

    return state;
}

coev::awaitable<int> Broker::Close()
{
    std::lock_guard<std::mutex> lock(m_lock);
    if (!m_conn)
    {
        co_return ErrNotConnected;
    }

    co_await m_done.get();
    int32_t err = m_conn.close();

    m_metric_registry->UnregisterAll();
    m_opened.store(false);
    co_return 0;
}

int32_t Broker::ID()
{
    return m_id;
}

std::string Broker::Addr()
{
    return m_addr;
}

std::string Broker::Rack()
{

    return m_rack;
}

coev::awaitable<int> Broker::GetMetadata(std::shared_ptr<MetadataRequest> request, std::shared_ptr<MetadataResponse> &response)
{
    response = std::make_shared<MetadataResponse>();
    response->m_version = request->m_version;
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::GetConsumerMetadata(std::shared_ptr<ConsumerMetadataRequest> request, std::shared_ptr<ConsumerMetadataResponse> &response)
{
    response = std::make_shared<ConsumerMetadataResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::FindCoordinator(std::shared_ptr<FindCoordinatorRequest> request, std::shared_ptr<FindCoordinatorResponse> &response)
{
    response = std::make_shared<FindCoordinatorResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::GetAvailableOffsets(std::shared_ptr<OffsetRequest> request, std::shared_ptr<OffsetResponse> &response)
{
    response = std::make_shared<OffsetResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AsyncProduce(std::shared_ptr<ProduceRequest> request, std::function<void(std::shared_ptr<ProduceResponse>, KError)> f)
{
    std::lock_guard<std::mutex> lock(m_lock);
    bool needAcks = request->m_acks != NoResponse;
    std::shared_ptr<ResponsePromise> promise;
    if (needAcks)
    {

        auto response = std::make_shared<ProduceResponse>();
        promise = std::make_shared<ResponsePromise>();
        promise->m_response = response;
        promise->m_handler = [f, response, request, this](std::string &packets, KError err)
        {
            if (err)
            {
                f({nullptr}, err);
                return;
            }
            bool decodeErr = versionedDecode(packets, std::dynamic_pointer_cast<VDecoder>(response), request->version(), m_metric_registry);
            if (decodeErr)
            {
                f({nullptr}, ErrDecodeError);
                return;
            }

            UandleThrottledResponse(response);
            f(response, ErrNoError);
        };
    }
    co_return co_await SendWithPromise(request, promise);
}

coev::awaitable<int> Broker::Produce(std::shared_ptr<ProduceRequest> request, std::shared_ptr<ProduceResponse> &response)
{
    if (request->m_acks == RequiredAcks::NoResponse)
    {
        co_return co_await SendAndReceive(request, nullptr);
    }
    else
    {
        response = std::make_shared<ProduceResponse>();
        co_return co_await SendAndReceive(request, response);
    }
}

coev::awaitable<int> Broker::Fetch(std::shared_ptr<FetchRequest> request, std::shared_ptr<FetchResponse> &response)
{
    if (m_fetch_rate)
    {
        m_fetch_rate->Mark(1);
    }
    if (m_broker_fetch_rate)
    {
        m_broker_fetch_rate->Mark(1);
    }
    response = std::make_shared<FetchResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CommitOffset(std::shared_ptr<OffsetCommitRequest> request, std::shared_ptr<OffsetCommitResponse> &response)
{
    response = std::make_shared<OffsetCommitResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::FetchOffset(std::shared_ptr<OffsetFetchRequest> request, std::shared_ptr<OffsetFetchResponse> &response)
{
    response = std::make_shared<OffsetFetchResponse>();
    response->m_version = request->m_version;
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::JoinGroup(std::shared_ptr<JoinGroupRequest> request, std::shared_ptr<JoinGroupResponse> &response)
{
    response = std::make_shared<JoinGroupResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::SyncGroup(std::shared_ptr<SyncGroupRequest> request, std::shared_ptr<SyncGroupResponse> &response)
{
    response = std::make_shared<SyncGroupResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::LeaveGroup(std::shared_ptr<LeaveGroupRequest> request, std::shared_ptr<LeaveGroupResponse> &response)
{
    response = std::make_shared<LeaveGroupResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::Heartbeat(std::shared_ptr<HeartbeatRequest> request, std::shared_ptr<HeartbeatResponse> &response)
{
    response = std::make_shared<HeartbeatResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ListGroups(std::shared_ptr<ListGroupsRequest> request, std::shared_ptr<ListGroupsResponse> &response)
{
    response = std::make_shared<ListGroupsResponse>();
    response->m_version = request->m_version;
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeGroups(std::shared_ptr<DescribeGroupsRequest> request, std::shared_ptr<DescribeGroupsResponse> &response)
{
    response = std::make_shared<DescribeGroupsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ApiVersions(std::shared_ptr<ApiVersionsRequest> request, std::shared_ptr<ApiVersionsResponse> &response)
{
    response = std::make_shared<ApiVersionsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CreateTopics(std::shared_ptr<CreateTopicsRequest> request, std::shared_ptr<CreateTopicsResponse> &response)
{
    response = std::make_shared<CreateTopicsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteTopics(std::shared_ptr<DeleteTopicsRequest> request, std::shared_ptr<DeleteTopicsResponse> &response)
{
    response = std::make_shared<DeleteTopicsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CreatePartitions(std::shared_ptr<CreatePartitionsRequest> request, std::shared_ptr<CreatePartitionsResponse> &response)
{
    response = std::make_shared<CreatePartitionsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterPartitionReassignments(std::shared_ptr<AlterPartitionReassignmentsRequest> request, std::shared_ptr<AlterPartitionReassignmentsResponse> &response)
{
    response = std::make_shared<AlterPartitionReassignmentsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ListPartitionReassignments(std::shared_ptr<ListPartitionReassignmentsRequest> request, std::shared_ptr<ListPartitionReassignmentsResponse> &response)
{
    response = std::make_shared<ListPartitionReassignmentsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ElectLeaders(std::shared_ptr<ElectLeadersRequest> request, std::shared_ptr<ElectLeadersResponse> &response)
{
    response = std::make_shared<ElectLeadersResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteRecords(std::shared_ptr<DeleteRecordsRequest> request, std::shared_ptr<DeleteRecordsResponse> &response)
{
    response = std::make_shared<DeleteRecordsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeAcls(std::shared_ptr<DescribeAclsRequest> request, std::shared_ptr<DescribeAclsResponse> &response)
{
    response = std::make_shared<DescribeAclsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CreateAcls(std::shared_ptr<CreateAclsRequest> request, std::shared_ptr<CreateAclsResponse> &response)
{
    response = std::make_shared<CreateAclsResponse>();
    int32_t err = co_await SendAndReceive(request, response);
    if (err)
    {
        co_return err;
    }
    std::vector<int> errs;
    for (auto &res : response->m_acl_creation_responses)
    {
        if (res->m_err != ErrNoError)
        {
            errs.push_back(res->m_err);
        }
    }
    if (!errs.empty())
    {
        co_return ErrCreateACLs;
    }
    co_return 0;
}

coev::awaitable<int> Broker::DeleteAcls(std::shared_ptr<DeleteAclsRequest> request, std::shared_ptr<DeleteAclsResponse> &response)
{
    response = std::make_shared<DeleteAclsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::InitProducerID(std::shared_ptr<InitProducerIDRequest> request, std::shared_ptr<InitProducerIDResponse> &response)
{
    response = std::make_shared<InitProducerIDResponse>();
    response->m_version = request->version();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AddPartitionsToTxn(std::shared_ptr<AddPartitionsToTxnRequest> request, std::shared_ptr<AddPartitionsToTxnResponse> &response)
{
    response = std::make_shared<AddPartitionsToTxnResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AddOffsetsToTxn(std::shared_ptr<AddOffsetsToTxnRequest> request, std::shared_ptr<AddOffsetsToTxnResponse> &response)
{
    response = std::make_shared<AddOffsetsToTxnResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::EndTxn(std::shared_ptr<EndTxnRequest> request, std::shared_ptr<EndTxnResponse> &response)
{
    response = std::make_shared<EndTxnResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::TxnOffsetCommit(std::shared_ptr<TxnOffsetCommitRequest> request, std::shared_ptr<TxnOffsetCommitResponse> &response)
{
    response = std::make_shared<TxnOffsetCommitResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeConfigs(std::shared_ptr<DescribeConfigsRequest> request, std::shared_ptr<DescribeConfigsResponse> &response)
{
    response = std::make_shared<DescribeConfigsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterConfigs(std::shared_ptr<AlterConfigsRequest> request, std::shared_ptr<AlterConfigsResponse> &response)
{
    response = std::make_shared<AlterConfigsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::IncrementalAlterConfigs(std::shared_ptr<IncrementalAlterConfigsRequest> request, std::shared_ptr<IncrementalAlterConfigsResponse> &response)
{

    response = std::make_shared<IncrementalAlterConfigsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteGroups(std::shared_ptr<DeleteGroupsRequest> request, std::shared_ptr<DeleteGroupsResponse> &response)
{

    response = std::make_shared<DeleteGroupsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteOffsets(std::shared_ptr<DeleteOffsetsRequest> request, std::shared_ptr<DeleteOffsetsResponse> &response)
{
    response = std::make_shared<DeleteOffsetsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeLogDirs(std::shared_ptr<DescribeLogDirsRequest> request, std::shared_ptr<DescribeLogDirsResponse> &response)
{

    response = std::make_shared<DescribeLogDirsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeUserScramCredentials(std::shared_ptr<DescribeUserScramCredentialsRequest> request, std::shared_ptr<DescribeUserScramCredentialsResponse> &response)
{
    response = std::make_shared<DescribeUserScramCredentialsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterUserScramCredentials(std::shared_ptr<AlterUserScramCredentialsRequest> request, std::shared_ptr<AlterUserScramCredentialsResponse> &response)
{
    response = std::make_shared<AlterUserScramCredentialsResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeClientQuotas(std::shared_ptr<DescribeClientQuotasRequest> request, std::shared_ptr<DescribeClientQuotasResponse> &response)
{
    if (response == nullptr)
    {
        response = std::make_shared<DescribeClientQuotasResponse>();
    }
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterClientQuotas(std::shared_ptr<AlterClientQuotasRequest> request, std::shared_ptr<AlterClientQuotasResponse> &response)
{
    response = std::make_shared<AlterClientQuotasResponse>();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ReadFull(std::string &buf, size_t &n)
{
    auto deadline = std::chrono::system_clock::now() + m_conf->Net.ReadTimeout;

    co_return 0;
}

coev::awaitable<int> Broker::Write(const std::string &buf, size_t &n)
{
    auto now = std::chrono::system_clock::now();
    // 设置写入超时逻辑
    // 写入数据
    co_return 0;
}

coev::awaitable<int> Broker::Send(std::shared_ptr<protocol_body> req, std::shared_ptr<protocol_body> res, std::shared_ptr<ResponsePromise> &promise)
{
    if (res)
    {
        promise = makeResponsePromise(res);
    }
    return SendWithPromise(req, promise);
}

std::shared_ptr<ResponsePromise> Broker::makeResponsePromise(std::shared_ptr<protocol_body> res)
{
    auto promise = std::make_shared<ResponsePromise>();
    promise->m_response = res;
    return promise;
}

coev::awaitable<int> Broker::SendWithPromise(std::shared_ptr<protocol_body> rb, std::shared_ptr<ResponsePromise> promise)
{
    if (!m_conn)
    {
        co_return ErrNotConnected;
    }
    int err = 0;
    if (m_client_session_reauthentication_time > 0 && CurrentUnixMilli() > m_client_session_reauthentication_time)
    {
        err = co_await AuthenticateViaSASLv1();
        if (err)
        {
            co_return err;
        }
    }
    err = co_await SendInternal(rb, promise);
    co_return err;
}

coev::awaitable<int> Broker::SendInternal(std::shared_ptr<protocol_body> rb, std::shared_ptr<ResponsePromise> promise)
{
    restrictApiVersion(rb, m_broker_api_versions);
    if (promise && promise->m_response)
    {
        promise->m_response->set_version(rb->version());
    }
    if (!m_conf->Version.IsAtLeast(rb->required_version()))
    {
        co_return ErrUnsupportedVersion;
    }
    auto req = std::make_shared<request>();
    req->m_correlation_id = m_correlation_id;
    req->m_client_id = m_conf->ClientID;
    req->m_body = rb;

    std::string buf;
    ::encode(req, buf, m_metric_registry);

    WaitIfThrottled();
    auto requestTime = std::chrono::system_clock::now();

    AddRequestInFlightMetrics(1);

    size_t bytes;
    auto err = co_await Write(buf, bytes);

    UpdateOutgoingCommunicationMetrics(bytes);

    UpdateProtocolMetrics(rb);
    if (err)
    {

        AddRequestInFlightMetrics(-1);
        co_return err;
    }

    m_correlation_id++;
    if (!promise)
    {
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - requestTime);
        UpdateRequestLatencyAndInFlightMetrics(latency);
        co_return 0;
    }

    promise->m_request_time = requestTime;
    promise->m_correlation_id = req->m_correlation_id;

    m_responses.set(promise);
    co_return 0;
}

coev::awaitable<int> HandleResponsePromise(std::shared_ptr<protocol_body> req, std::shared_ptr<protocol_body> res, std::shared_ptr<ResponsePromise> promise, std::shared_ptr<metrics::Registry> m_metric_registry)
{
    auto packets = co_await promise->m_packets.get();
    if (packets.empty())
    {
        auto err = co_await promise->m_errors.get();
        co_return err;
    }
    co_return versionedDecode(packets, res, req->version(), m_metric_registry);
}

int Broker::decode(PDecoder &pd, int16_t version)
{
    int32_t err = pd.getInt32(m_id);
    if (err)
    {
        return err;
    }

    std::string host;
    err = pd.getString(host);
    if (err)
    {
        return err;
    }

    int32_t port;
    err = pd.getInt32(port);
    if (err)
    {
        return err;
    }

    if (version >= 1)
    {
        std::string rack_;
        err = pd.getNullableString(rack_);
        if (err)
        {
            return err;
        }
    }

    auto addr = net::JoinHostPort(host, port);
    std::string host_;
    int portstr;
    std::tie(host_, portstr) = net::SplitHostPort(addr);
    if (host_.empty())
    {
        return 1;
    }

    int32_t emptyArray;
    err = pd.getEmptyTaggedFieldArray(emptyArray);
    return err;
}

int Broker::encode(PEncoder &pe, int16_t version)
{

    auto [host, port] = net::SplitHostPort(m_addr);
    if (host.empty())
    {
        return 1;
    }

    pe.putInt32(m_id);

    int32_t err = pe.putString(host);
    if (err)
    {
        return err;
    }

    pe.putInt32(port);
    if (version >= 1)
    {
        err = pe.putNullableString(m_rack);
        if (err)
        {
            return err;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

coev::awaitable<int> Broker::SesponseReceiver()
{
    int dead;
    while (true)
    {
        auto promise = co_await m_responses.get();

        if (dead)
        {
            AddRequestInFlightMetrics(-1);
            std::string _;
            promise->Handle(_, (KError)dead);
            continue;
        }

        int8_t headerLength = getHeaderLength(promise->m_response->headerVersion());
        std::string header;
        header.resize(headerLength);
        size_t bytesReadHeader;
        auto err = co_await ReadFull(header, bytesReadHeader);
        auto m_request_latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - promise->m_request_time);

        if (err)
        {

            UpdateIncomingCommunicationMetrics(bytesReadHeader, m_request_latency);
            dead = err;
            std::string _;
            promise->Handle(_, (KError)dead);
            continue;
        }

        auto decodedHeader = std::make_shared<responseHeader>();
        err = versionedDecode(header, std::dynamic_pointer_cast<VDecoder>(decodedHeader), promise->m_response->headerVersion(),
                              m_metric_registry);
        if (err)
        {

            UpdateIncomingCommunicationMetrics(bytesReadHeader, m_request_latency);
            dead = err;
            std::string _;
            promise->Handle(_, (KError)dead);
            continue;
        }

        if (decodedHeader->m_correlation_id != promise->m_correlation_id)
        {

            UpdateIncomingCommunicationMetrics(bytesReadHeader, m_request_latency);
            dead = 1;
            std::string _;
            promise->Handle(_, (KError)dead);
            continue;
        }

        std::string buf;
        buf.resize(decodedHeader->m_length - headerLength + 4);
        size_t bytesReadBody;
        err = co_await ReadFull(buf, bytesReadBody);

        UpdateIncomingCommunicationMetrics(bytesReadHeader + bytesReadBody, m_request_latency);
        if (err)
        {
            dead = err;
            std::string _;
            promise->Handle(_, (KError)dead);
            continue;
        }
        promise->Handle(buf, ErrNoError);
    }

    m_done.set(true);
    co_return 0;
}

coev::awaitable<int> Broker::SendAndReceiveApiVersions(int16_t v, std::shared_ptr<ApiVersionsResponse> &response)
{
    auto rb = std::make_shared<ApiVersionsRequest>();
    rb->m_version = v;
    rb->m_client_software_name = defaultClientSoftwareName;
    rb->m_client_software_version = version();

    auto req = std::make_shared<request>();
    req->m_correlation_id = m_correlation_id;
    req->m_client_id = m_conf->ClientID;
    req->m_body = rb;

    std::string buf;
    ::encode(std::dynamic_pointer_cast<IEncoder>(req), buf, m_metric_registry);

    auto requestTime = std::chrono::system_clock::now();

    AddRequestInFlightMetrics(1);

    size_t bytes;
    auto err = co_await Write(buf, bytes);

    UpdateOutgoingCommunicationMetrics(bytes);
    if (err)
    {

        AddRequestInFlightMetrics(-1);
        co_return err;
    }

    m_correlation_id++;
    std::string header;
    header.resize(8);
    size_t headerBytes = 8;
    err = co_await ReadFull(header, headerBytes);
    if (err)
    {

        AddRequestInFlightMetrics(-1);
        co_return err;
    }

    uint32_t length = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
    std::string payload;
    payload.resize(length - 4);
    size_t n;
    err = co_await ReadFull(payload, n);
    if (err)
    {
        AddRequestInFlightMetrics(-1);
        co_return err;
    }

    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - requestTime);

    UpdateIncomingCommunicationMetrics(n + 8, latency);

    response = std::make_shared<ApiVersionsResponse>();
    response->m_version = rb->version();
    err = versionedDecode(payload, response, rb->version(),
                          m_metric_registry);
    if (err)
    {
        co_return err;
    }

    int16_t kerr = response->m_error_code;
    if (kerr != 0)
    {
        co_return kerr;
    }
    co_return 0;
}

coev::awaitable<int> Broker::AuthenticateViaSASLv0()
{
    std::string mechanism =
        m_conf->Net.SASL.Mechanism;
    if (mechanism == "SCRAM-SHA-256" || mechanism == "SCRAM-SHA-512")
    {
        return SendAndReceiveSASLSCRAMv0();
    }
    else if (mechanism == "GSSAPI")
    {
        return SendAndReceiveKerberos();
    }
    else
    {
        return SendAndReceiveSASLPlainAuthV0();
    }
}

coev::awaitable<int> Broker::DefaultAuthSendReceiver(const std::string &authBytes, std::shared_ptr<SaslAuthenticateResponse> &authenticateResponse)

{
    auto authenticateRequest = CreateSaslAuthenticateRequest(authBytes);
    authenticateResponse = std::make_shared<SaslAuthenticateResponse>();
    auto prom = makeResponsePromise(authenticateResponse);
    auto err = co_await SendInternal(authenticateRequest, prom);
    if (err)
    {
        co_return err;
    }
    err = co_await HandleResponsePromise(authenticateRequest, authenticateResponse, prom, m_metric_registry);
    if (err)
    {
        co_return err;
    }
    if (authenticateResponse->m_err != 0)
    {
        co_return authenticateResponse->m_err;
    }

    computeSaslSessionLifetime(authenticateResponse);
    co_return 0;
};
coev::awaitable<int> Broker::AuthenticateViaSASLv1()
{

    if (m_conf->Net.SASL.Handshake)
    {
        auto handshakeRequest = std::make_shared<SaslHandshakeRequest>();
        handshakeRequest->m_mechanism = m_conf->Net.SASL.Mechanism;
        handshakeRequest->m_version = m_conf->Net.SASL.Version;
        auto handshakeResponse = std::make_shared<SaslHandshakeResponse>();
        auto prom = makeResponsePromise(handshakeResponse);
        int err = co_await SendInternal(handshakeRequest, prom);
        if (err)
        {
            co_return err;
        }
        err = co_await HandleResponsePromise(handshakeRequest, handshakeResponse, prom, m_metric_registry);
        if (err)
        {
            co_return err;
        }
        if (handshakeResponse->m_err != 0)
        {
            co_return handshakeResponse->m_err;
        }
    }

    std::string mechanism = m_conf->Net.SASL.Mechanism;
    if (mechanism == "GSSAPI")
    {

        m_kerberos_authenticator->m_config = m_conf->Net.SASL.GSSAPI;
        if (!m_kerberos_authenticator->m_new_kerberos_client_func)
        {
            m_kerberos_authenticator->m_new_kerberos_client_func = NewKerberosClient;
        }
        co_return co_await m_kerberos_authenticator->AuthorizeV2(shared_from_this(), [this](auto req, auto res) -> auto
                                                              { return DefaultAuthSendReceiver(req, res); });
    }
    else if (mechanism == "OAUTHBEARER")
    {
        auto provider = m_conf->Net.SASL.TokenProvider_;
        co_return co_await SendAndReceiveSASLOAuth(provider, [this](auto req, auto res) -> auto
                                                   { return DefaultAuthSendReceiver(req, res); });
    }
    else if (mechanism == "SCRAM-SHA-256" || mechanism == "SCRAM-SHA-512")
    {
        co_return co_await SendAndReceiveSASLSCRAMv1(
            m_conf->Net.SASL.ScramClientGeneratorFunc(), [this](auto req, auto res) -> auto
            { return DefaultAuthSendReceiver(req, res); });
    }
    else
    {

        co_return co_await SendAndReceiveSASLPlainAuthV1([this](auto req, auto res) -> auto
                                                         { return DefaultAuthSendReceiver(req, res); });
    }
}

coev::awaitable<int> Broker::SendAndReceiveKerberos()
{

    m_kerberos_authenticator->m_config = m_conf->Net.SASL.GSSAPI;
    if (!m_kerberos_authenticator->m_new_kerberos_client_func)
    {

        m_kerberos_authenticator->m_new_kerberos_client_func = NewKerberosClient;
    }
    co_return co_await m_kerberos_authenticator->Authorize(shared_from_this());
}

coev::awaitable<int> Broker::SendAndReceiveSASLHandshake(const std::string &saslType, int16_t version)
{
    auto rb = std::make_shared<SaslHandshakeRequest>();
    rb->m_mechanism = saslType;
    rb->m_version = version;

    auto req = std::make_shared<request>();
    req->m_correlation_id = m_correlation_id;
    req->m_client_id = m_conf->ClientID;
    req->m_body = rb;

    std::string buf;
    ::encode(std::dynamic_pointer_cast<IEncoder>(req), buf, m_metric_registry);

    auto requestTime = std::chrono::system_clock::now();
    AddRequestInFlightMetrics(1);

    size_t bytes;
    auto err = co_await Write(buf, bytes);
    UpdateOutgoingCommunicationMetrics(bytes);
    if (err)
    {

        AddRequestInFlightMetrics(-1);
        co_return err;
    }

    m_correlation_id++;
    std::string header;
    header.resize(8);
    size_t headerBytes = 8;
    err = co_await ReadFull(header, headerBytes);
    if (err)
    {
        AddRequestInFlightMetrics(-1);
        co_return err;
    }

    uint32_t length = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
    std::string payload;
    payload.resize(length - 4);
    size_t n = length - 4;
    err = co_await ReadFull(payload, n);
    if (err)
    {

        AddRequestInFlightMetrics(-1);
        co_return err;
    }

    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - requestTime);

    UpdateIncomingCommunicationMetrics(n + 8, latency);

    auto res = std::make_shared<SaslHandshakeResponse>();
    err = versionedDecode(payload, res, 0, m_metric_registry);
    if (err)
    {
        co_return err;
    }

    if (res->m_err != 0)
    {
        co_return res->m_err;
    }
    co_return 0;
}

coev::awaitable<int> Broker::SendAndReceiveSASLPlainAuthV0()
{
    if (m_conf->Net.SASL.Handshake)
    {
        int32_t handshakeErr = co_await SendAndReceiveSASLHandshake("PLAIN", m_conf->Net.SASL.Version);
        if (handshakeErr)
        {
            co_return handshakeErr;
        }
    }

    size_t length = m_conf->Net.SASL.AuthIdentity.length() + 1 +
                    m_conf->Net.SASL.User.length() + 1 +
                    m_conf->Net.SASL.Password.length();
    std::string authBytes;
    authBytes.resize(length + 4);
    uint32_t len = length;
    authBytes[0] = (len >> 24) & 0xFF;
    authBytes[1] = (len >> 16) & 0xFF;
    authBytes[2] = (len >> 8) & 0xFF;
    authBytes[3] = len & 0xFF;

    size_t offset = 4;
    memcpy(&authBytes[offset],
           m_conf->Net.SASL.AuthIdentity.c_str(),
           m_conf->Net.SASL.AuthIdentity.length());
    offset +=
        m_conf->Net.SASL.AuthIdentity.length();
    authBytes[offset++] = 0;
    memcpy(&authBytes[offset],
           m_conf->Net.SASL.User.c_str(),
           m_conf->Net.SASL.User.length());
    offset +=
        m_conf->Net.SASL.User.length();
    authBytes[offset++] = 0;
    memcpy(&authBytes[offset],
           m_conf->Net.SASL.Password.c_str(),
           m_conf->Net.SASL.Password.length());

    auto requestTime = std::chrono::system_clock::now();

    AddRequestInFlightMetrics(1);

    size_t bytesWritten;
    int32_t err = co_await Write(authBytes, bytesWritten);

    UpdateOutgoingCommunicationMetrics(bytesWritten);
    if (err)
    {

        AddRequestInFlightMetrics(-1);
        co_return err;
    }

    std::string header;
    header.resize(4);
    size_t n = 4;
    err = co_await ReadFull(header, n);
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - requestTime);

    UpdateIncomingCommunicationMetrics(n, latency);
    if (err)
    {
        co_return err;
    }
    co_return 0;
}

coev::awaitable<int> Broker::SendAndReceiveSASLPlainAuthV1(AuthSendReceiver authSendReceiver)
{
    std::string authStr = m_conf->Net.SASL.AuthIdentity + "\x00" + m_conf->Net.SASL.User + "\x00" + m_conf->Net.SASL.Password;
    std::string authBytes(authStr.begin(), authStr.end());
    std::shared_ptr<SaslAuthenticateResponse> response = std::make_shared<SaslAuthenticateResponse>();
    return authSendReceiver(authBytes, response);
}

int64_t Broker::CurrentUnixMilli()
{
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

coev::awaitable<int> Broker::SendAndReceiveSASLOAuth(std::shared_ptr<AccessTokenProvider> provider, AuthSendReceiver authSendReceiver)
{
    std::shared_ptr<AccessToken> token;
    int err = provider->Token(token);
    if (err != ErrNoError)
    {
        co_return err;
    }

    std::string message;
    err = BuildClientFirstMessage(token, message);
    if (err != ErrNoError)
    {
        co_return err;
    }
    std::shared_ptr<SaslAuthenticateResponse> res;
    err = co_await authSendReceiver(message, res);
    if (err != ErrNoError)
    {
        co_return err;
    }

    bool isChallenge = !res->m_sasl_auth_bytes.empty();
    if (isChallenge)
    {
        std::string challenge = {'\x01'};
        err = co_await authSendReceiver(challenge, res);
        if (err != ErrNoError)
        {
            co_return err;
        }
    }
    co_return 0;
}

coev::awaitable<int> Broker::SendAndReceiveSASLSCRAMv0()
{
    int32_t err = co_await SendAndReceiveSASLHandshake(m_conf->Net.SASL.Mechanism, 0);
    if (err)
    {
        co_return err;
    }

    auto scramClient = m_conf->Net.SASL.ScramClientGeneratorFunc();
    err = scramClient->Begin(m_conf->Net.SASL.User, m_conf->Net.SASL.Password, m_conf->Net.SASL.ScramAuthzID);
    if (err)
    {
        co_return 1;
    }

    std::string msg;
    err = scramClient->Step("", msg);
    if (err)
    {
        co_return 1;
    }

    while (!scramClient->Done())
    {
        auto requestTime = std::chrono::system_clock::now();

        AddRequestInFlightMetrics(1);

        size_t length = msg.length();
        std::string authBytes;
        authBytes.resize(length + 4);
        uint32_t len = length;
        authBytes[0] = (len >> 24) & 0xFF;
        authBytes[1] = (len >> 16) & 0xFF;
        authBytes[2] = (len >> 8) & 0xFF;
        authBytes[3] = len & 0xFF;
        memcpy(&authBytes[4], msg.c_str(), length);

        size_t bytesWritten;
        err = co_await Write(authBytes, bytesWritten);

        UpdateOutgoingCommunicationMetrics(length + 4);
        if (err)
        {

            AddRequestInFlightMetrics(-1);
            co_return err;
        }

        m_correlation_id++;
        std::string header;
        header.resize(4);
        size_t headerBytes = 4;
        err = co_await ReadFull(header, headerBytes);
        if (err)
        {

            AddRequestInFlightMetrics(-1);
            co_return err;
        }

        uint32_t payloadLen = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
        std::string payload;
        payload.resize(payloadLen);
        size_t n = payloadLen;
        err = co_await ReadFull(payload, n);
        if (err)
        {
            AddRequestInFlightMetrics(-1);
            co_return err;
        }

        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - requestTime);

        UpdateIncomingCommunicationMetrics(n + 4, latency);

        err = scramClient->Step(std::string(payload.begin(), payload.end()), msg);
        if (err)
        {
            co_return err;
        }
    }
    co_return 0;
}

coev::awaitable<int> Broker::SendAndReceiveSASLSCRAMv1(std::shared_ptr<SCRAMClient> scramClient, AuthSendReceiver authSendReceiver)
{
    int err = scramClient->Begin(m_conf->Net.SASL.User, m_conf->Net.SASL.Password, m_conf->Net.SASL.ScramAuthzID);
    if (err)
    {
        co_return err;
    }

    std::string msg;
    err = scramClient->Step("", msg);
    if (err)
    {
        co_return err;
    }

    while (!scramClient->Done())
    {
        std::shared_ptr<SaslAuthenticateResponse> res;
        err = co_await authSendReceiver(std::string(msg.begin(), msg.end()), res);
        if (err)
        {
            co_return err;
        }

        err = scramClient->Step(std::string(res->m_sasl_auth_bytes.begin(), res->m_sasl_auth_bytes.end()), msg);
        if (err)
        {
            co_return err;
        }
    }
    co_return 0;
}

std::shared_ptr<SaslAuthenticateRequest> Broker::CreateSaslAuthenticateRequest(const std::string &msg)
{
    auto authenticateRequest = std::make_shared<SaslAuthenticateRequest>();
    authenticateRequest->m_sasl_auth_bytes = msg;
    if (m_conf->Version.IsAtLeast(V2_2_0_0))
    {
        authenticateRequest->m_version = 1;
    }
    return authenticateRequest;
}

int Broker::BuildClientFirstMessage(std::shared_ptr<AccessToken> token, std::string &message)
{
    if (!token)
    {
        message.clear();
        return 1;
    }

    std::string ext;
    if (!token->m_extensions.empty())
    {
        if (token->m_extensions.find("auth") != token->m_extensions.end())
        {
            message.clear();
            return 1;
        }
        ext = "\x01" + mapToString(token->m_extensions, "=", "\x01");
    }

    auto msg = "n,,\x01auth=Bearer " + token->m_token + ext + "\x01\x01";
    message.assign(msg.begin(), msg.end());
    return 0;
}

std::string Broker::mapToString(const std::map<std::string, std::string> &extensions, const std::string &keyValSep, const std::string &elemSep)
{
    std::vector<std::string> buf;
    for (auto &[k, v] : extensions)
    {
        buf.push_back(k + keyValSep + v);
    }
    std::sort(buf.begin(), buf.end());
    std::string result;
    for (size_t i = 0; i < buf.size(); ++i)
    {
        if (i > 0)
        {
            result += elemSep;
        }
        result += buf[i];
    }
    return result;
}

void Broker::computeSaslSessionLifetime(std::shared_ptr<SaslAuthenticateResponse> res)
{
    if (res->m_session_lifetime_ms > 0)
    {
        int64_t positiveSessionLifetimeMs = res->m_session_lifetime_ms;
        int64_t authenticationEndMs = CurrentUnixMilli();
        double pctWindowFactorToTakeNetworkLatencyAndClockDriftIntoAccount = 0.85;
        double pctWindowJitterToAvoidReauthenticationStormAcrossManyChannelsSimultaneously = 0.10;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, pctWindowJitterToAvoidReauthenticationStormAcrossManyChannelsSimultaneously);

        double pctToUse = pctWindowFactorToTakeNetworkLatencyAndClockDriftIntoAccount + dis(gen);
        int64_t sessionLifetimeMsToUse = static_cast<int64_t>(positiveSessionLifetimeMs * pctToUse);

        m_client_session_reauthentication_time = authenticationEndMs + sessionLifetimeMsToUse;
    }
    else
    {
        m_client_session_reauthentication_time = 0;
    }
}

void Broker::UpdateIncomingCommunicationMetrics(int bytes, std::chrono::milliseconds m_request_latencyMS)
{

    UpdateRequestLatencyAndInFlightMetrics(m_request_latencyMS);

    m_response_rate->Mark(1);
    if (m_broker_response_rate)
    {
        m_broker_response_rate->Mark(1);
    }

    m_incoming_byte_rate->Mark(bytes);
    if (m_broker_incoming_byte_rate)
    {
        m_broker_incoming_byte_rate->Mark(bytes);
    }

    m_response_size->Update(bytes);
    if (m_broker_response_size)
    {

        m_broker_response_size->Update(bytes);
    }
}

void Broker::UpdateRequestLatencyAndInFlightMetrics(std::chrono::milliseconds m_request_latencyMS)
{
    int64_t m_request_latencyInMs = m_request_latencyMS.count();

    m_request_latency->Update(m_request_latencyInMs);
    if (m_broker_request_latency)
    {
        m_broker_request_latency->Update(m_request_latencyInMs);
    }

    AddRequestInFlightMetrics(-1);
}

void Broker::AddRequestInFlightMetrics(int64_t i)
{

    m_requests_in_flight->Inc(i);
    if (m_broker_requests_in_flight)
    {

        m_broker_requests_in_flight->Inc(i);
    }
}

void Broker::UpdateOutgoingCommunicationMetrics(int bytes)
{

    m_request_rate->Mark(1);
    if (m_broker_request_rate)
    {
        m_broker_request_rate->Mark(1);
    }
    m_outgoing_byte_rate->Mark(bytes);
    if (m_broker_outgoing_byte_rate)
    {
        m_broker_outgoing_byte_rate->Mark(bytes);
    }

    m_request_size->Update(bytes);
    if (m_broker_request_size)
    {

        m_broker_request_size->Update(bytes);
    }
}

void Broker::UpdateProtocolMetrics(std::shared_ptr<protocol_body> rb)
{
    int16_t key = rb->key();
    auto &_protocolRequestsRate = m_protocol_requests_rate[key];
    if (!_protocolRequestsRate)
    {
        std::string name = "protocol-requests-rate-" + std::to_string(key);
        _protocolRequestsRate = metrics::GetOrRegisterMeter(name, m_metric_registry);
    }
    _protocolRequestsRate->Mark(1);

    auto &_m_broker_protocol_requests_rate = m_broker_protocol_requests_rate[key];
    if (!_m_broker_protocol_requests_rate)
    {
        std::string name = "protocol-requests-rate-" + std::to_string(key);
        _m_broker_protocol_requests_rate = metrics::GetOrRegisterMeter(name, m_metric_registry);
    }
    _m_broker_protocol_requests_rate->Mark(1);
}

void Broker::UandleThrottledResponse(std::shared_ptr<protocol_body> resp)
{
    auto throttledResponse = std::dynamic_pointer_cast<throttle_support>(resp);
    if (!throttledResponse)
    {
        return;
    }
    auto throttleTime = throttledResponse->throttleTime();
    if (throttleTime.count() == 0)
    {
        return;
    }

    SetThrottle(throttleTime);

    UpdateThrottleMetric(throttleTime);
}

void Broker::SetThrottle(std::chrono::milliseconds throttleTime)
{
    std::lock_guard<std::mutex> lock(m_throttle_timer_lock);
    {
        // 停止现有计时器
    }
    // 创建新计时器
}

void Broker::WaitIfThrottled()
{
    std::lock_guard<std::mutex> lock(m_throttle_timer_lock);
    {
        // 等待计时器
    }
}

void Broker::UpdateThrottleMetric(std::chrono::milliseconds throttleTime)
{
    if (m_broker_throttle_time)
    {
        int64_t throttleTimeInMs = throttleTime.count();
        m_broker_throttle_time->Update(throttleTimeInMs);
    }
}

void Broker::RegisterMetrics()
{

    m_broker_incoming_byte_rate = RegisterMeter("incoming-byte-rate");
    m_broker_request_rate = RegisterMeter("request-rate");
    m_broker_fetch_rate = RegisterMeter("consumer-fetch-rate");
    m_broker_request_size = RegisterHistogram("request-size");
    m_broker_request_latency = RegisterHistogram("request-latency-in-ms");
    m_broker_outgoing_byte_rate = RegisterMeter("outgoing-byte-rate");
    m_broker_response_rate = RegisterMeter("response-rate");
    m_broker_response_size = RegisterHistogram("response-size");
    m_broker_requests_in_flight = RegisterCounter("requests-in-flight");
    m_broker_throttle_time = RegisterHistogram("throttle-time-in-ms");

    m_broker_protocol_requests_rate.clear();
}

std::shared_ptr<metrics::Meter> Broker::RegisterMeter(const std::string &name)
{
    std::string nameForBroker = metrics::GetMetricNameForBroker(name, shared_from_this());
    return metrics::GetOrRegisterMeter(nameForBroker, m_metric_registry);
}

std::shared_ptr<metrics::Histogram> Broker::RegisterHistogram(const std::string &name)
{
    std::string nameForBroker = metrics::GetMetricNameForBroker(name, shared_from_this());
    return metrics::GetOrRegisterHistogram(nameForBroker, m_metric_registry);
}

std::shared_ptr<metrics::Counter> Broker::RegisterCounter(const std::string &name)
{
    std::string nameForBroker = metrics::GetMetricNameForBroker(name, shared_from_this());
    return metrics::GetOrRegisterCounter(nameForBroker, m_metric_registry);
}
coev::awaitable<int> Broker::SendOffsetCommit(std::shared_ptr<OffsetCommitRequest> req, std::shared_ptr<OffsetCommitResponse> &resp, std::shared_ptr<ResponsePromise> &promise)
{
    return Send(req, resp, promise);
}

std::shared_ptr<tls::Config> ValidServerNameTLS(const std::string &addr, std::shared_ptr<tls::Config> cfg)
{
    if (!cfg)
    {
        cfg = std::make_shared<tls::Config>();
    }
    if (!cfg->ServerName.empty())
    {
        return cfg;
    }
    size_t colon = addr.find(':');
    if (colon != std::string::npos)
    {
        cfg->ServerName = addr.substr(0, colon);
    }
    else
    {
        cfg->ServerName = addr;
    }
    return cfg;
}

void Broker::safeAsyncClose()
{
    auto b = shared_from_this();
    m_task << [b]() -> coev::awaitable<void>
    {
        if (!b->Connected())
        {
            auto err = co_await b->Close();
            if (err != ErrNoError)
            {
                Logger::Println("Error closing broker", b->ID(), ":", err);
            }
        }
        co_return;
    }();
}
