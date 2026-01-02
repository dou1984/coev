#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <random>
#include <cstring>
#include <coev.h>
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

Broker::Broker(const std::string &addr) : m_ID(-1), m_Addr(addr), m_Opened(false), m_CorrelationID(0), clientSessionReauthenticationTime(0)
{
}
Broker::Broker(int id, const std::string &addr) : m_ID(id), m_Addr(addr), m_Opened(false), m_CorrelationID(0), clientSessionReauthenticationTime(0)
{
}
coev::awaitable<int> Broker::Open(std::shared_ptr<Config> conf)
{
    bool expected = false;
    if (!m_Opened.compare_exchange_strong(expected, true))
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
    std::lock_guard<std::mutex> lock(m_Lock);
    if (!metricRegistry)
    {
        metricRegistry = metrics::NewRegistry();
    }
    m_Conf = conf;
    m_task << [this]() -> coev::awaitable<int>
    {
        std::lock_guard<std::mutex> lock(m_Lock);

        auto [host, port] = net::SplitHostPort(m_Addr);
        auto fd = co_await m_Conn.connect(host.data(), port);
        if (fd == INVALID)
        {
            co_return ErrNotConnected;
        }

        incomingByteRate = metrics::GetOrRegisterMeter("incoming-byte-rate", metricRegistry);

        requestRate = metrics::GetOrRegisterMeter("request-rate", metricRegistry);

        fetchRate = metrics::GetOrRegisterMeter("consumer-fetch-rate", metricRegistry);

        requestSize = metrics::GetOrRegisterHistogram("request-size", metricRegistry);

        requestLatency = metrics::GetOrRegisterHistogram("request-latency-in-ms", metricRegistry);

        outgoingByteRate = metrics::GetOrRegisterMeter("outgoing-byte-rate", metricRegistry);

        responseRate = metrics::GetOrRegisterMeter("response-rate", metricRegistry);

        responseSize = metrics::GetOrRegisterHistogram("response-size", metricRegistry);

        requestsInFlight = metrics::GetOrRegisterCounter("requests-in-flight", metricRegistry);

        if (m_ID >= 0)
        {
            RegisterMetrics();
        }
        int err = ErrNoError;
        if (m_Conf->ApiVersionsRequest)
        {
            std::shared_ptr<ApiVersionsResponse> apiVersionsResponse;
            err = co_await SendAndReceiveApiVersions(3, apiVersionsResponse);
            if (err)
            {
                int16_t maxVersion = 0;
                if (apiVersionsResponse)
                {
                    for (auto &k : apiVersionsResponse->ApiKeys)
                    {
                        if (k.ApiKey == apiKeyApiVersions)
                        {
                            maxVersion = k.MaxVersion;
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

                brokerAPIVersions.clear();
                for (auto &key : apiVersionsResponse->ApiKeys)
                {
                    brokerAPIVersions.emplace(key.ApiKey, ApiVersionRange(key.MinVersion, key.MaxVersion));
                }
            }
        }

        bool useSaslV0 = m_Conf->Net.SASL.Version == 0;
        if (m_Conf->Net.SASL.Enable && useSaslV0)
        {

            err = co_await AuthenticateViaSASLv0();
            if (err)
            {
                m_Conn.close();
                m_Opened.store(false);
                co_return ErrPermitForbidden;
            }
        }

        m_task << SesponseReceiver();
        if (m_Conf->Net.SASL.Enable && !useSaslV0)
        {
            err = co_await AuthenticateViaSASLv1();
            if (err)
            {
                co_await done.get();
                int32_t closeErr = m_Conn.close();
                m_Opened.store(false);
                co_return err;
            }
        }
    }();

    co_return 0;
}

int Broker::ResponseSize()
{
    std::lock_guard<std::mutex> lock(m_Lock);
    return responses.size();
}

bool Broker::Connected()
{
    std::lock_guard<std::mutex> lock(m_Lock);
    return m_Conn;
}

int Broker::TLSConnectionState()
{
    std::lock_guard<std::mutex> lock(m_Lock);
    tls::ConnectionState state;
    if (!m_Conn)
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
    std::lock_guard<std::mutex> lock(m_Lock);
    if (!m_Conn)
    {
        co_return ErrNotConnected;
    }

    co_await done.get();
    int32_t err = m_Conn.close();

    metricRegistry->UnregisterAll();
    m_Opened.store(false);
    co_return 0;
}

int32_t Broker::ID()
{
    return m_ID;
}

std::string Broker::Addr()
{
    return m_Addr;
}

std::string Broker::Rack()
{

    return m_Rack;
}

coev::awaitable<int> Broker::GetMetadata(std::shared_ptr<MetadataRequest> request, std::shared_ptr<MetadataResponse> &response)
{
    response = std::make_shared<MetadataResponse>();
    response->Version = request->Version;
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

coev::awaitable<int> Broker::AsyncProduce(std::shared_ptr<ProduceRequest> request, std::function<void(std::shared_ptr<ProduceResponse>, KError)> cb)
{
    std::lock_guard<std::mutex> lock(m_Lock);
    bool needAcks = request->Acks_ != NoResponse;
    std::shared_ptr<ResponsePromise> promise;
    if (needAcks)
    {

        auto response = std::make_shared<ProduceResponse>();
        promise = std::make_shared<ResponsePromise>();
        promise->Response = response;
        promise->Handler = [cb, response, request, this](std::string &packets, KError err)
        {
            if (err)
            {
                cb({nullptr}, err);
                return;
            }
            bool decodeErr = versionedDecode(packets, std::dynamic_pointer_cast<VDecoder>(response), request->version(), metricRegistry);
            if (decodeErr)
            {
                cb({nullptr}, ErrDecodeError);
                return;
            }

            UandleThrottledResponse(response);
            cb(response, ErrNoError);
        };
    }
    co_return co_await SendWithPromise(request, promise);
}

coev::awaitable<int> Broker::Produce(std::shared_ptr<ProduceRequest> request, std::shared_ptr<ProduceResponse> &response)
{
    if (request->Acks_ == RequiredAcks::NoResponse)
    {
        return SendAndReceive(request, nullptr);
    }
    else
    {
        response = std::make_shared<ProduceResponse>();
        return SendAndReceive(request, response);
    }
}

coev::awaitable<int> Broker::Fetch(std::shared_ptr<FetchRequest> request, std::shared_ptr<FetchResponse> &response)
{
    if (fetchRate)
    {
        fetchRate->Mark(1);
    }
    if (brokerFetchRate)
    {

        brokerFetchRate->Mark(1);
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
    response->Version = request->Version;
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
    response->Version = request->Version;
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
    for (auto &res : response->AclCreationResponses_)
    {
        if (res->Err != ErrNoError)
        {
            errs.push_back(res->Err);
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
    if (response == nullptr)
    {
        response = std::make_shared<InitProducerIDResponse>();
    }
    response->Version = request->version();
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AddPartitionsToTxn(std::shared_ptr<AddPartitionsToTxnRequest> request, std::shared_ptr<AddPartitionsToTxnResponse> &response)
{
    if (response == nullptr)
    {
        response = std::make_shared<AddPartitionsToTxnResponse>();
    }
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AddOffsetsToTxn(std::shared_ptr<AddOffsetsToTxnRequest> request, std::shared_ptr<AddOffsetsToTxnResponse> &response)
{
    if (response == nullptr)
    {
        response = std::make_shared<AddOffsetsToTxnResponse>();
    }
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::EndTxn(std::shared_ptr<EndTxnRequest> request, std::shared_ptr<EndTxnResponse> &response)
{
    if (response == nullptr)
    {
        response = std::make_shared<EndTxnResponse>();
    }
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::TxnOffsetCommit(std::shared_ptr<TxnOffsetCommitRequest> request, std::shared_ptr<TxnOffsetCommitResponse> &response)
{
    if (response == nullptr)
    {
        response = std::make_shared<TxnOffsetCommitResponse>();
    }
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeConfigs(std::shared_ptr<DescribeConfigsRequest> request, std::shared_ptr<DescribeConfigsResponse> &response)
{
    if (response == nullptr)
    {
        response = std::make_shared<DescribeConfigsResponse>();
    }
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterConfigs(std::shared_ptr<AlterConfigsRequest> request, std::shared_ptr<AlterConfigsResponse> &response)
{
    if (response == nullptr)
    {
        response = std::make_shared<AlterConfigsResponse>();
    }
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::IncrementalAlterConfigs(std::shared_ptr<IncrementalAlterConfigsRequest> request, std::shared_ptr<IncrementalAlterConfigsResponse> &response)
{
    if (response == nullptr)
    {
        response = std::make_shared<IncrementalAlterConfigsResponse>();
    }
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteGroups(std::shared_ptr<DeleteGroupsRequest> request, std::shared_ptr<DeleteGroupsResponse> &response)
{
    if (response == nullptr)
    {
        response = std::make_shared<DeleteGroupsResponse>();
    }
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteOffsets(std::shared_ptr<DeleteOffsetsRequest> request, std::shared_ptr<DeleteOffsetsResponse> &response)
{
    if (response == nullptr)
    {
        response = std::make_shared<DeleteOffsetsResponse>();
    }
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeLogDirs(std::shared_ptr<DescribeLogDirsRequest> request, std::shared_ptr<DescribeLogDirsResponse> &response)
{
    if (response == nullptr)
    {
        response = std::make_shared<DescribeLogDirsResponse>();
    }
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeUserScramCredentials(std::shared_ptr<DescribeUserScramCredentialsRequest> request, std::shared_ptr<DescribeUserScramCredentialsResponse> &response)
{
    if (response == nullptr)
    {
        response = std::make_shared<DescribeUserScramCredentialsResponse>();
    }
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterUserScramCredentials(std::shared_ptr<AlterUserScramCredentialsRequest> request, std::shared_ptr<AlterUserScramCredentialsResponse> &response)
{
    if (response == nullptr)
    {
        response = std::make_shared<AlterUserScramCredentialsResponse>();
    }
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
    auto deadline = std::chrono::system_clock::now() + m_Conf->Net.ReadTimeout;
    // 设置读取超时逻辑
    // 读取数据到buf
    co_return 0;
}

coev::awaitable<int> Broker::Write(const std::string &buf, size_t &n)
{
    auto now = std::chrono::system_clock::now();
    // 设置写入超时逻辑
    // 写入数据
    co_return 0;
}

coev::awaitable<int> Broker::Send(std::shared_ptr<protocolBody> req, std::shared_ptr<protocolBody> res, std::shared_ptr<ResponsePromise> &promise)
{
    if (res)
    {
        promise = makeResponsePromise(res);
    }
    return SendWithPromise(req, promise);
}

std::shared_ptr<ResponsePromise> Broker::makeResponsePromise(std::shared_ptr<protocolBody> res)
{
    auto promise = std::make_shared<ResponsePromise>();
    promise->Response = res;
    return promise;
}

coev::awaitable<int> Broker::SendWithPromise(std::shared_ptr<protocolBody> rb, std::shared_ptr<ResponsePromise> promise)
{
    if (!m_Conn)
    {
        co_return ErrNotConnected;
    }
    if (clientSessionReauthenticationTime > 0 && CurrentUnixMilli() > clientSessionReauthenticationTime)
    {
        int32_t err = co_await AuthenticateViaSASLv1();
        if (err)
        {
            co_return err;
        }
    }
    co_return co_await SendInternal(rb, promise);
}

coev::awaitable<int> Broker::SendInternal(std::shared_ptr<protocolBody> rb, std::shared_ptr<ResponsePromise> promise)
{
    restrictApiVersion(rb, brokerAPIVersions);
    if (promise && promise->Response)
    {
        promise->Response->setVersion(rb->version());
    }
    if (!m_Conf->Version.IsAtLeast(rb->requiredVersion()))
    {
        co_return ErrUnsupportedVersion;
    }
    auto req = std::make_shared<request>();
    req->correlationID = m_CorrelationID;
    req->clientID = m_Conf->ClientID;
    req->body = rb;

    std::string buf;
    ::encode(req, buf, metricRegistry);

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

    m_CorrelationID++;
    if (!promise)
    {
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - requestTime);
        UpdateRequestLatencyAndInFlightMetrics(latency);
        co_return 0;
    }

    promise->RequestTime = requestTime;
    promise->CorrelationID = req->correlationID;

    responses.set(promise);
    co_return 0;
}

coev::awaitable<int> HandleResponsePromise(std::shared_ptr<protocolBody> req, std::shared_ptr<protocolBody> res, std::shared_ptr<ResponsePromise> promise, std::shared_ptr<metrics::Registry> metricRegistry)
{
    auto packets = co_await promise->Packets.get();
    if (packets.empty())
    {
        auto err = co_await promise->Errors.get();
        co_return err;
    }
    co_return versionedDecode(packets, res, req->version(), metricRegistry);
}

int Broker::decode(PDecoder &pd, int16_t version)
{
    int32_t err = pd.getInt32(m_ID);
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

    auto [host, port] = net::SplitHostPort(m_Addr);
    if (host.empty())
    {
        return 1;
    }

    pe.putInt32(m_ID);

    int32_t err = pe.putString(host);
    if (err)
    {
        return err;
    }

    pe.putInt32(port);
    if (version >= 1)
    {
        err = pe.putNullableString(m_Rack);
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
        auto promise = co_await responses.get();

        if (dead)
        {
            AddRequestInFlightMetrics(-1);
            std::string _;
            promise->Handle(_, (KError)dead);
            continue;
        }

        int8_t headerLength = getHeaderLength(promise->Response->headerVersion());
        std::string header;
        header.resize(headerLength);
        size_t bytesReadHeader;
        auto err = co_await ReadFull(header, bytesReadHeader);
        auto requestLatency = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - promise->RequestTime);

        if (err)
        {

            UpdateIncomingCommunicationMetrics(bytesReadHeader, requestLatency);
            dead = err;
            std::string _;
            promise->Handle(_, (KError)dead);
            continue;
        }

        auto decodedHeader = std::make_shared<responseHeader>();
        err = versionedDecode(header, std::dynamic_pointer_cast<VDecoder>(decodedHeader), promise->Response->headerVersion(),
                              metricRegistry);
        if (err)
        {

            UpdateIncomingCommunicationMetrics(bytesReadHeader, requestLatency);
            dead = err;
            std::string _;
            promise->Handle(_, (KError)dead);
            continue;
        }

        if (decodedHeader->correlationID != promise->CorrelationID)
        {

            UpdateIncomingCommunicationMetrics(bytesReadHeader, requestLatency);
            dead = 1;
            std::string _;
            promise->Handle(_, (KError)dead);
            continue;
        }

        std::string buf;
        buf.resize(decodedHeader->length - headerLength + 4);
        size_t bytesReadBody;
        err = co_await ReadFull(buf, bytesReadBody);

        UpdateIncomingCommunicationMetrics(bytesReadHeader + bytesReadBody, requestLatency);
        if (err)
        {
            dead = err;
            std::string _;
            promise->Handle(_, (KError)dead);
            continue;
        }
        promise->Handle(buf, ErrNoError);
    }

    done.set(true);
    co_return 0;
}

coev::awaitable<int> Broker::SendAndReceiveApiVersions(int16_t v, std::shared_ptr<ApiVersionsResponse> &response)
{
    auto rb = std::make_shared<ApiVersionsRequest>();
    rb->Version = v;
    rb->ClientSoftwareName = defaultClientSoftwareName;
    rb->ClientSoftwareVersion = version();

    auto req = std::make_shared<request>();
    req->correlationID = m_CorrelationID;
    req->clientID = m_Conf->ClientID;
    req->body = rb;

    std::string buf;
    ::encode(std::dynamic_pointer_cast<IEncoder>(req), buf, metricRegistry);

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

    m_CorrelationID++;
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
    response->Version = rb->version();
    err = versionedDecode(payload, response, rb->version(),
                          metricRegistry);
    if (err)
    {
        co_return err;
    }

    int16_t kerr = response->ErrorCode;
    if (kerr != 0)
    {
        co_return kerr;
    }
    co_return 0;
}

coev::awaitable<int> Broker::AuthenticateViaSASLv0()
{
    std::string mechanism =
        m_Conf->Net.SASL.Mechanism;
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
    err = co_await HandleResponsePromise(authenticateRequest, authenticateResponse, prom, metricRegistry);
    if (err)
    {
        co_return err;
    }
    if (authenticateResponse->Err != 0)
    {
        co_return authenticateResponse->Err;
    }

    computeSaslSessionLifetime(authenticateResponse);
    co_return 0;
};
coev::awaitable<int> Broker::AuthenticateViaSASLv1()
{

    if (m_Conf->Net.SASL.Handshake)
    {
        auto handshakeRequest = std::make_shared<SaslHandshakeRequest>();
        handshakeRequest->Mechanism = m_Conf->Net.SASL.Mechanism;
        handshakeRequest->Version = m_Conf->Net.SASL.Version;
        auto handshakeResponse = std::make_shared<SaslHandshakeResponse>();
        auto prom = makeResponsePromise(handshakeResponse);
        int err = co_await SendInternal(handshakeRequest, prom);
        if (err)
        {
            co_return err;
        }
        err = co_await HandleResponsePromise(handshakeRequest, handshakeResponse, prom, metricRegistry);
        if (err)
        {
            co_return err;
        }
        if (handshakeResponse->Err != 0)
        {
            co_return handshakeResponse->Err;
        }
    }

    std::string mechanism = m_Conf->Net.SASL.Mechanism;
    if (mechanism == "GSSAPI")
    {

        kerberosAuthenticator->Config = m_Conf->Net.SASL.GSSAPI;
        if (!kerberosAuthenticator->NewKerberosClientFunc)
        {
            kerberosAuthenticator->NewKerberosClientFunc = NewKerberosClient;
        }
        co_return co_await kerberosAuthenticator->AuthorizeV2(shared_from_this(), [this](auto req, auto res) -> auto
                                                              { return DefaultAuthSendReceiver(req, res); });
    }
    else if (mechanism == "OAUTHBEARER")
    {
        auto provider = m_Conf->Net.SASL.TokenProvider_;
        co_return co_await SendAndReceiveSASLOAuth(provider, [this](auto req, auto res) -> auto
                                                   { return DefaultAuthSendReceiver(req, res); });
    }
    else if (mechanism == "SCRAM-SHA-256" || mechanism == "SCRAM-SHA-512")
    {
        co_return co_await SendAndReceiveSASLSCRAMv1(
            m_Conf->Net.SASL.ScramClientGeneratorFunc(), [this](auto req, auto res) -> auto
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

    kerberosAuthenticator->Config = m_Conf->Net.SASL.GSSAPI;
    if (!kerberosAuthenticator->NewKerberosClientFunc)
    {

        kerberosAuthenticator->NewKerberosClientFunc = NewKerberosClient;
    }
    co_return co_await kerberosAuthenticator->Authorize(shared_from_this());
}

coev::awaitable<int> Broker::SendAndReceiveSASLHandshake(const std::string &saslType, int16_t version)
{
    auto rb = std::make_shared<SaslHandshakeRequest>();
    rb->Mechanism = saslType;
    rb->Version = version;

    auto req = std::make_shared<request>();
    req->correlationID = m_CorrelationID;
    req->clientID = m_Conf->ClientID;
    req->body = rb;

    std::string buf;
    ::encode(std::dynamic_pointer_cast<IEncoder>(req), buf, metricRegistry);

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

    m_CorrelationID++;
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
    err = versionedDecode(payload, res, 0, metricRegistry);
    if (err)
    {
        co_return err;
    }

    if (res->Err != 0)
    {
        co_return res->Err;
    }
    co_return 0;
}

coev::awaitable<int> Broker::SendAndReceiveSASLPlainAuthV0()
{
    if (m_Conf->Net.SASL.Handshake)
    {
        int32_t handshakeErr = co_await SendAndReceiveSASLHandshake("PLAIN", m_Conf->Net.SASL.Version);
        if (handshakeErr)
        {
            co_return handshakeErr;
        }
    }

    size_t length = m_Conf->Net.SASL.AuthIdentity.length() + 1 +
                    m_Conf->Net.SASL.User.length() + 1 +
                    m_Conf->Net.SASL.Password.length();
    std::string authBytes;
    authBytes.resize(length + 4);
    uint32_t len = length;
    authBytes[0] = (len >> 24) & 0xFF;
    authBytes[1] = (len >> 16) & 0xFF;
    authBytes[2] = (len >> 8) & 0xFF;
    authBytes[3] = len & 0xFF;

    size_t offset = 4;
    memcpy(&authBytes[offset],
           m_Conf->Net.SASL.AuthIdentity.c_str(),
           m_Conf->Net.SASL.AuthIdentity.length());
    offset +=
        m_Conf->Net.SASL.AuthIdentity.length();
    authBytes[offset++] = 0;
    memcpy(&authBytes[offset],
           m_Conf->Net.SASL.User.c_str(),
           m_Conf->Net.SASL.User.length());
    offset +=
        m_Conf->Net.SASL.User.length();
    authBytes[offset++] = 0;
    memcpy(&authBytes[offset],
           m_Conf->Net.SASL.Password.c_str(),
           m_Conf->Net.SASL.Password.length());

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
    std::string authStr = m_Conf->Net.SASL.AuthIdentity + "\x00" + m_Conf->Net.SASL.User + "\x00" + m_Conf->Net.SASL.Password;
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

    bool isChallenge = !res->SaslAuthBytes.empty();
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
    int32_t err = co_await SendAndReceiveSASLHandshake(m_Conf->Net.SASL.Mechanism, 0);
    if (err)
    {
        co_return err;
    }

    auto scramClient = m_Conf->Net.SASL.ScramClientGeneratorFunc();
    err = scramClient->Begin(m_Conf->Net.SASL.User, m_Conf->Net.SASL.Password, m_Conf->Net.SASL.ScramAuthzID);
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

        m_CorrelationID++;
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
    int err = scramClient->Begin(m_Conf->Net.SASL.User, m_Conf->Net.SASL.Password, m_Conf->Net.SASL.ScramAuthzID);
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

        err = scramClient->Step(std::string(res->SaslAuthBytes.begin(), res->SaslAuthBytes.end()), msg);
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
    authenticateRequest->SaslAuthBytes = msg;
    if (m_Conf->Version.IsAtLeast(V2_2_0_0))
    {
        authenticateRequest->Version = 1;
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
    if (!token->Extensions.empty())
    {
        if (token->Extensions.find("auth") != token->Extensions.end())
        {
            message.clear();
            return 1;
        }
        ext = "\x01" + mapToString(token->Extensions, "=", "\x01");
    }

    auto msg = "n,,\x01auth=Bearer " + token->Token + ext + "\x01\x01";
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
    if (res->SessionLifetimeMs > 0)
    {
        int64_t positiveSessionLifetimeMs = res->SessionLifetimeMs;
        int64_t authenticationEndMs = CurrentUnixMilli();
        double pctWindowFactorToTakeNetworkLatencyAndClockDriftIntoAccount = 0.85;
        double pctWindowJitterToAvoidReauthenticationStormAcrossManyChannelsSimultaneously = 0.10;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, pctWindowJitterToAvoidReauthenticationStormAcrossManyChannelsSimultaneously);

        double pctToUse = pctWindowFactorToTakeNetworkLatencyAndClockDriftIntoAccount + dis(gen);
        int64_t sessionLifetimeMsToUse = static_cast<int64_t>(positiveSessionLifetimeMs * pctToUse);

        clientSessionReauthenticationTime = authenticationEndMs + sessionLifetimeMsToUse;
    }
    else
    {
        clientSessionReauthenticationTime = 0;
    }
}

void Broker::UpdateIncomingCommunicationMetrics(int bytes, std::chrono::milliseconds requestLatencyMS)
{

    UpdateRequestLatencyAndInFlightMetrics(requestLatencyMS);

    responseRate->Mark(1);
    if (brokerResponseRate)
    {
        brokerResponseRate->Mark(1);
    }

    incomingByteRate->Mark(bytes);
    if (brokerIncomingByteRate)
    {
        brokerIncomingByteRate->Mark(bytes);
    }

    responseSize->Update(bytes);
    if (brokerResponseSize)
    {

        brokerResponseSize->Update(bytes);
    }
}

void Broker::UpdateRequestLatencyAndInFlightMetrics(std::chrono::milliseconds requestLatencyMS)
{
    int64_t requestLatencyInMs = requestLatencyMS.count();

    requestLatency->Update(requestLatencyInMs);
    if (brokerRequestLatency)
    {
        brokerRequestLatency->Update(requestLatencyInMs);
    }

    AddRequestInFlightMetrics(-1);
}

void Broker::AddRequestInFlightMetrics(int64_t i)
{

    requestsInFlight->Inc(i);
    if (brokerRequestsInFlight)
    {

        brokerRequestsInFlight->Inc(i);
    }
}

void Broker::UpdateOutgoingCommunicationMetrics(int bytes)
{

    requestRate->Mark(1);
    if (brokerRequestRate)
    {
        brokerRequestRate->Mark(1);
    }
    outgoingByteRate->Mark(bytes);
    if (brokerOutgoingByteRate)
    {
        brokerOutgoingByteRate->Mark(bytes);
    }

    requestSize->Update(bytes);
    if (brokerRequestSize)
    {

        brokerRequestSize->Update(bytes);
    }
}

void Broker::UpdateProtocolMetrics(std::shared_ptr<protocolBody> rb)
{
    int16_t key = rb->key();
    auto &_protocolRequestsRate = protocolRequestsRate[key];
    if (!_protocolRequestsRate)
    {
        std::string name = "protocol-requests-rate-" + std::to_string(key);
        _protocolRequestsRate = metrics::GetOrRegisterMeter(name, metricRegistry);
    }
    _protocolRequestsRate->Mark(1);

    auto &_brokerProtocolRequestsRate = brokerProtocolRequestsRate[key];
    if (!_brokerProtocolRequestsRate)
    {
        std::string name = "protocol-requests-rate-" + std::to_string(key);
        _brokerProtocolRequestsRate = GetOrRegisterMeter(name, metricRegistry);
    }
    _brokerProtocolRequestsRate->Mark(1);
}

void Broker::UandleThrottledResponse(std::shared_ptr<protocolBody> resp)
{
    auto throttledResponse = std::dynamic_pointer_cast<throttleSupport>(resp);
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
    std::lock_guard<std::mutex> lock(throttleTimerLock);
    {
        // 停止现有计时器
    }
    // 创建新计时器
}

void Broker::WaitIfThrottled()
{
    std::lock_guard<std::mutex> lock(throttleTimerLock);
    {
        // 等待计时器
    }
}

void Broker::UpdateThrottleMetric(std::chrono::milliseconds throttleTime)
{
    if (brokerThrottleTime)
    {
        int64_t throttleTimeInMs = throttleTime.count();
        brokerThrottleTime->Update(throttleTimeInMs);
    }
}

void Broker::RegisterMetrics()
{

    brokerIncomingByteRate = RegisterMeter("incoming-byte-rate");

    brokerRequestRate = RegisterMeter("request-rate");

    brokerFetchRate = RegisterMeter("consumer-fetch-rate");

    brokerRequestSize = RegisterHistogram("request-size");

    brokerRequestLatency = RegisterHistogram("request-latency-in-ms");

    brokerOutgoingByteRate = RegisterMeter("outgoing-byte-rate");

    brokerResponseRate = RegisterMeter("response-rate");

    brokerResponseSize = RegisterHistogram("response-size");

    brokerRequestsInFlight = RegisterCounter("requests-in-flight");

    brokerThrottleTime = RegisterHistogram("throttle-time-in-ms");

    brokerProtocolRequestsRate.clear();
}

std::shared_ptr<metrics::Meter> Broker::RegisterMeter(const std::string &name)
{
    std::string nameForBroker = metrics::GetMetricNameForBroker(name, shared_from_this());
    return metrics::GetOrRegisterMeter(nameForBroker, metricRegistry);
}

std::shared_ptr<metrics::Histogram> Broker::RegisterHistogram(const std::string &name)
{
    std::string nameForBroker = metrics::GetMetricNameForBroker(name, shared_from_this());
    return GetOrRegisterHistogram(nameForBroker, metricRegistry);
}

std::shared_ptr<metrics::Counter> Broker::RegisterCounter(const std::string &name)
{
    std::string nameForBroker = metrics::GetMetricNameForBroker(name, shared_from_this());
    return metrics::GetOrRegisterCounter(nameForBroker, metricRegistry);
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
    auto c = std::make_shared<tls::Config>(*cfg);
    size_t colon = addr.find(':');
    if (colon != std::string::npos)
    {
        c->ServerName = addr.substr(0, colon);
    }
    else
    {
        c->ServerName = addr;
    }
    return c;
}

void Broker::safeAsyncClose()
{
    auto b = shared_from_this();
    co_start << [b]() -> coev::awaitable<int>
    {
        if (!b->Connected())
        {
            auto err = co_await b->Close();
            if (err != ErrNoError)
            {
                Logger::Println("Error closing broker", b->ID(), ":", err);
            }
        }
        co_return 0;
    }();
}
