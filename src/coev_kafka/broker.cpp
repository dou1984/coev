
#include <chrono>
#include <cstring>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <regex>
#include <coev/coev.h>
#include <coev_dns/parse_dns.h>
#include "undefined.h"
#include "broker.h"
#include "access_token.h"
#include "kerberos_client.h"
#include "response_header.h"
#include "scram_client.h"
#include "sleep_for.h"

int8_t GetHeaderLength(int16_t header_version)
{
    if (header_version < 1)
    {
        return 8;
    }
    else
    {
        return 9;
    }
}

Broker::Broker(const std::string &addr) : m_id(-1), m_addr(addr), m_rack(""), m_correlation_id(0), m_session_reauthentication_time(0)
{
   
}
Broker::Broker(int id, const std::string &addr) : m_id(id), m_addr(addr), m_rack(""), m_correlation_id(0), m_session_reauthentication_time(0)
{
   
}
Broker::Broker() : m_id(0), m_addr(""), m_rack(""), m_correlation_id(0), m_session_reauthentication_time(0)
{
  
}
Broker::~Broker()
{
    LOG_CORE("broker %p closed", this);
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
int Broker::decode(packet_decoder &pd, int16_t version)
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

    if (host == "localhost")
    {
        host = "127.0.0.1";
        // host = "0.0.0.0"
    }
    m_addr = host + ":" + std::to_string(port);

    if (version >= 1)
    {
        err = pd.getNullableString(m_rack);
        if (err)
        {
            return err;
        }
    }

    if (version >= 5)
    {
        int32_t emptyArray;
        err = pd.getEmptyTaggedFieldArray(emptyArray);
        if (err)
        {
            return err;
        }
    }

    return 0;
}

int Broker::encode(packet_encoder &pe, int16_t version) const
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

coev::awaitable<int> Broker::Open(std::shared_ptr<Config> conf)
{
    if (!conf)
    {
        conf = std::make_shared<Config>();
    }
    if (!conf->Validate())
    {
        co_return INVALID;
    }
    m_conf = conf;
    auto err = co_await _Open();
    while (m_opened.resume())
    {
    }
    co_return err;
}

coev::awaitable<int> Broker::_Open()
{
    if (m_conn == nullptr)
    {
        m_conn = std::make_unique<Connect>();
    }
    if (m_conn->IsOpened())
    {
        LOG_CORE("conn is opened");
        co_return 0;
    }
    if (m_conn->IsOpening())
    {
        co_await m_opened.suspend();
        if (m_conn->IsClosed())
        {
            co_return INVALID;
        }
    }
    if (m_addr.empty())
    {
        LOG_CORE("addr is empty");
        co_return INVALID;
    }
    auto [host, port] = net::SplitHostPort(m_addr);
    static std::regex ipv4_re("^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}$");
    static std::regex ipv6_re("^(\\[)?([0-9a-fA-F:]+)(\\])?$|:");
    if (!(std::regex_match(host, ipv4_re) || std::regex_match(host, ipv6_re)))
    {
        auto tmp = host;
        auto err = co_await coev::parse_dns(tmp, host);
        if (err)
        {
            LOG_CORE("parse_dns failed");
            co_return INVALID;
        }
    }
    else if (host.size() >= 2 && host[0] == '[' && host.back() == ']')
    {
        host = host.substr(1, host.size() - 2);
    }

    auto fd = co_await m_conn->Dial(host.data(), port);
    if (fd == INVALID)
    {
        LOG_CORE("connect to %s:%d failed", host.data(), port);
        co_return INVALID;
    }

    int err = ErrNoError;
    if (m_conf->ApiVersionsRequest)
    {
        ResponsePromise<ApiVersionsResponse> apiVersionsResponse;
        err = co_await SendAndReceiveApiVersions(3, apiVersionsResponse);
        if (err)
        {
            err = co_await SendAndReceiveApiVersions(0, apiVersionsResponse);
            if (err)
            {
                m_conn->Close();
                LOG_CORE("connect to %s:%d failed due to ApiVersionsRequest failure", host.data(), port);
                co_return INVALID;
            }
        }
        m_broker_api_versions.clear();
        for (auto &key : apiVersionsResponse.m_response->m_api_keys)
        {
            m_broker_api_versions.emplace(key.m_api_key, ApiVersionRange(key.m_min_version, key.m_max_version));
        }
    }

    bool useSaslV0 = m_conf->Net.SASL.Version == 0;
    if (m_conf->Net.SASL.Enable && useSaslV0)
    {
        err = co_await AuthenticateViaSASLv0();
        if (err)
        {
            m_conn->Close();
            LOG_CORE("connect to %s:%d failed", host.data(), port);
            co_return INVALID;
        }
    }

    if (m_conf->Net.SASL.Enable && !useSaslV0)
    {
        err = co_await AuthenticateViaSASLv1();
        if (err)
        {
            m_conn->Close();
            LOG_CORE("connect to %s:%d failed", host.data(), port)
            co_return INVALID;
        }
    }

    co_return 0;
}

bool Broker::Connected()
{
    if (m_conn == nullptr)
    {
        return false;
    }
    return m_conn->IsOpened();
}

int Broker::TLSConnectionState()
{
    if (m_conn == nullptr || !m_conn->IsOpened())
    {
        return 0;
    }

    // if (auto bconn = (buf::Conn *)(conn))
    // {
    //     return bconn->ConnectionState();
    // }
    // if (auto tc = (tls::Conn *)(conn))
    // {
    //     return tc->ConnectionState();
    // }

    return 0;
}

int Broker::Close()
{
    if (m_conn == nullptr)
    {
        return ErrNotConnected;
    }
    // Always call m_conn->Close() regardless of current state
    // This ensures proper cleanup of libev resources
    return m_conn->Close();
}

coev::awaitable<int> Broker::GetMetadata(std::shared_ptr<MetadataRequest> request, ResponsePromise<MetadataResponse> &response)
{
    LOG_CORE("Sending MetadataRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received MetadataResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}
coev::awaitable<int> Broker::GetConsumerMetadata(std::shared_ptr<ConsumerMetadataRequest> request, ResponsePromise<ConsumerMetadataResponse> &response)
{
    LOG_CORE("Sending ConsumerMetadataRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received ConsumerMetadataResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::FindCoordinator(std::shared_ptr<FindCoordinatorRequest> request, ResponsePromise<FindCoordinatorResponse> &response)
{
    LOG_CORE("Sending FindCoordinatorRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received FindCoordinatorResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::GetAvailableOffsets(std::shared_ptr<OffsetRequest> request, ResponsePromise<OffsetResponse> &response)
{
    LOG_CORE("Sending OffsetRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received OffsetResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::Produce(std::shared_ptr<ProduceRequest> request, ResponsePromise<ProduceResponse> &response)
{
    if (request->m_acks == RequiredAcks::NoResponse)
    {
        LOG_CORE("Sending ProduceRequest, version: %d, correlation_id: %d, acks: %d", request->version(), m_correlation_id, request->m_acks);
        defer(LOG_CORE("Received ProduceResponse, correlation_id: %d", response.m_correlation_id));
        int32_t err = co_await SendAndReceive(request, response);
        co_return err;
    }
    else
    {
        LOG_CORE("Sending ProduceRequest, version: %d, correlation_id: %d, acks: %d", request->version(), m_correlation_id, request->m_acks);
        defer(LOG_CORE("Received ProduceResponse, correlation_id: %d", response.m_correlation_id));
        co_return co_await SendAndReceive(request, response);
    }
}

coev::awaitable<int> Broker::Fetch(std::shared_ptr<FetchRequest> request, ResponsePromise<FetchResponse> &response)
{
    LOG_CORE("Sending FetchRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received FetchResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CommitOffset(std::shared_ptr<OffsetCommitRequest> request, ResponsePromise<OffsetCommitResponse> &response)
{
    LOG_CORE("Sending OffsetCommitRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received OffsetCommitResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::FetchOffset(std::shared_ptr<OffsetFetchRequest> request, ResponsePromise<OffsetFetchResponse> &response)
{
    LOG_CORE("Sending OffsetFetchRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received OffsetFetchResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::JoinGroup(std::shared_ptr<JoinGroupRequest> request, ResponsePromise<JoinGroupResponse> &response)
{
    LOG_CORE("Sending JoinGroupRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received JoinGroupResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::SyncGroup(std::shared_ptr<SyncGroupRequest> request, ResponsePromise<SyncGroupResponse> &response)
{
    LOG_CORE("Sending SyncGroupRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received SyncGroupResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::LeaveGroup(std::shared_ptr<LeaveGroupRequest> request, ResponsePromise<LeaveGroupResponse> &response)
{
    LOG_CORE("Sending LeaveGroupRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received LeaveGroupResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::Heartbeat(std::shared_ptr<HeartbeatRequest> request, ResponsePromise<HeartbeatResponse> &response)
{
    LOG_CORE("Sending HeartbeatRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received HeartbeatResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ListGroups(std::shared_ptr<ListGroupsRequest> request, ResponsePromise<ListGroupsResponse> &response)
{
    LOG_CORE("Sending ListGroupsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received ListGroupsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeGroups(std::shared_ptr<DescribeGroupsRequest> request, ResponsePromise<DescribeGroupsResponse> &response)
{
    LOG_CORE("Sending DescribeGroupsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received DescribeGroupsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ApiVersions(std::shared_ptr<ApiVersionsRequest> request, ResponsePromise<ApiVersionsResponse> &response)
{
    LOG_CORE("Sending ApiVersionsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received ApiVersionsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CreateTopics(std::shared_ptr<CreateTopicsRequest> request, ResponsePromise<CreateTopicsResponse> &response)
{
    LOG_CORE("Sending CreateTopicsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received CreateTopicsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteTopics(std::shared_ptr<DeleteTopicsRequest> request, ResponsePromise<DeleteTopicsResponse> &response)
{
    LOG_CORE("Sending DeleteTopicsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received DeleteTopicsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CreatePartitions(std::shared_ptr<CreatePartitionsRequest> request, ResponsePromise<CreatePartitionsResponse> &response)
{
    LOG_CORE("Sending CreatePartitionsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received CreatePartitionsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterPartitionReassignments(std::shared_ptr<AlterPartitionReassignmentsRequest> request, ResponsePromise<AlterPartitionReassignmentsResponse> &response)
{
    LOG_CORE("Sending AlterPartitionReassignmentsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received AlterPartitionReassignmentsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ListPartitionReassignments(std::shared_ptr<ListPartitionReassignmentsRequest> request, ResponsePromise<ListPartitionReassignmentsResponse> &response)
{
    LOG_CORE("Sending ListPartitionReassignmentsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received ListPartitionReassignmentsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ElectLeaders(std::shared_ptr<ElectLeadersRequest> request, ResponsePromise<ElectLeadersResponse> &response)
{
    LOG_CORE("Sending ElectLeadersRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received ElectLeadersResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteRecords(std::shared_ptr<DeleteRecordsRequest> request, ResponsePromise<DeleteRecordsResponse> &response)
{
    LOG_CORE("Sending DeleteRecordsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received DeleteRecordsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeAcls(std::shared_ptr<DescribeAclsRequest> request, ResponsePromise<DescribeAclsResponse> &response)
{
    LOG_CORE("Sending DescribeAclsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received DescribeAclsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CreateAcls(std::shared_ptr<CreateAclsRequest> request, ResponsePromise<CreateAclsResponse> &response)
{
    LOG_CORE("Sending CreateAclsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received CreateAclsResponse, correlation_id: %d", response.m_correlation_id));
    int32_t err = co_await SendAndReceive(request, response);
    if (err)
    {
        co_return err;
    }
    std::vector<int> errs;
    for (auto &res : response.m_response->m_acl_creation_responses)
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

coev::awaitable<int> Broker::DeleteAcls(std::shared_ptr<DeleteAclsRequest> request, ResponsePromise<DeleteAclsResponse> &response)
{
    LOG_CORE("Sending DeleteAclsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received DeleteAclsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::InitProducerID(std::shared_ptr<InitProducerIDRequest> request, ResponsePromise<InitProducerIDResponse> &response)
{
    LOG_CORE("Sending InitProducerIDRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received InitProducerIDResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AddPartitionsToTxn(std::shared_ptr<AddPartitionsToTxnRequest> request, ResponsePromise<AddPartitionsToTxnResponse> &response)
{
    LOG_CORE("Sending AddPartitionsToTxnRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received AddPartitionsToTxnResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AddOffsetsToTxn(std::shared_ptr<AddOffsetsToTxnRequest> request, ResponsePromise<AddOffsetsToTxnResponse> &response)
{
    LOG_CORE("Sending AddOffsetsToTxnRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received AddOffsetsToTxnResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::EndTxn(std::shared_ptr<EndTxnRequest> request, ResponsePromise<EndTxnResponse> &response)
{
    LOG_CORE("Sending EndTxnRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received EndTxnResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::TxnOffsetCommit(std::shared_ptr<TxnOffsetCommitRequest> request, ResponsePromise<TxnOffsetCommitResponse> &response)
{
    LOG_CORE("Sending TxnOffsetCommitRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received TxnOffsetCommitResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeConfigs(std::shared_ptr<DescribeConfigsRequest> request, ResponsePromise<DescribeConfigsResponse> &response)
{
    LOG_CORE("Sending DescribeConfigsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received DescribeConfigsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterConfigs(std::shared_ptr<AlterConfigsRequest> request, ResponsePromise<AlterConfigsResponse> &response)
{
    LOG_CORE("Sending AlterConfigsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received AlterConfigsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::IncrementalAlterConfigs(std::shared_ptr<IncrementalAlterConfigsRequest> request, ResponsePromise<IncrementalAlterConfigsResponse> &response)
{
    LOG_CORE("Sending IncrementalAlterConfigsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received IncrementalAlterConfigsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteGroups(std::shared_ptr<DeleteGroupsRequest> request, ResponsePromise<DeleteGroupsResponse> &response)
{
    LOG_CORE("Sending DeleteGroupsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received DeleteGroupsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteOffsets(std::shared_ptr<DeleteOffsetsRequest> request, ResponsePromise<DeleteOffsetsResponse> &response)
{
    LOG_CORE("Sending DeleteOffsetsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received DeleteOffsetsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeLogDirs(std::shared_ptr<DescribeLogDirsRequest> request, ResponsePromise<DescribeLogDirsResponse> &response)
{
    LOG_CORE("Sending DescribeLogDirsRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received DescribeLogDirsResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeUserScramCredentials(std::shared_ptr<DescribeUserScramCredentialsRequest> req, ResponsePromise<DescribeUserScramCredentialsResponse> &res)
{
    LOG_CORE("Sending DescribeUserScramCredentialsRequest, version: %d, correlation_id: %d", req->version(), m_correlation_id);
    defer(LOG_CORE("Received DescribeUserScramCredentialsResponse, correlation_id: %d", res.m_correlation_id));
    return SendAndReceive(req, res);
}

coev::awaitable<int> Broker::AlterUserScramCredentials(std::shared_ptr<AlterUserScramCredentialsRequest> req, ResponsePromise<AlterUserScramCredentialsResponse> &res)
{
    LOG_CORE("Sending AlterUserScramCredentialsRequest, version: %d, correlation_id: %d", req->version(), m_correlation_id);
    defer(LOG_CORE("Received AlterUserScramCredentialsResponse, correlation_id: %d", res.m_correlation_id));
    return SendAndReceive(req, res);
}

coev::awaitable<int> Broker::DescribeClientQuotas(std::shared_ptr<DescribeClientQuotasRequest> request, ResponsePromise<DescribeClientQuotasResponse> &response)
{
    LOG_CORE("Sending DescribeClientQuotasRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received DescribeClientQuotasResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterClientQuotas(std::shared_ptr<AlterClientQuotasRequest> request, ResponsePromise<AlterClientQuotasResponse> &response)
{
    LOG_CORE("Sending AlterClientQuotasRequest, version: %d, correlation_id: %d", request->version(), m_correlation_id);
    defer(LOG_CORE("Received AlterClientQuotasResponse, correlation_id: %d", response.m_correlation_id));
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ReadFull(std::string &buf, size_t n)
{
    if (m_conn == nullptr)
    {
        co_return INVALID;
    }
    if (m_conn->IsClosed())
    {
        co_return INVALID;
    }
    if (m_conn->IsOpening())
    {
        co_await m_opened.suspend();
        if (m_conn->IsClosed())
        {
            co_return INVALID;
        }
    }
    if (buf.size() < n)
    {
        buf.resize(n);
    }
    int err = co_await m_conn->ReadFull(buf, n);
    if (err != ErrNoError)
    {
        LOG_CORE("ReadFull %d %d %s", err, errno, strerror(errno));
        co_return INVALID;
    }
    co_return 0;
}

coev::awaitable<int> Broker::Write(const std::string &buf)
{
    if (m_conn == nullptr)
    {
        co_return INVALID;
    }
    auto now = std::chrono::system_clock::now();
    if (m_conn->IsClosed())
    {
        co_return INVALID;
    }
    if (m_conn->IsOpening())
    {
        co_await m_opened.suspend();
        if (m_conn->IsClosed())
        {
            co_return ErrNotConnected;
        }
    }
    co_return co_await m_conn->Write(buf);
}

coev::awaitable<int> Broker::AuthenticateViaSASLv0()
{
    std::string mechanism = m_conf->Net.SASL.Mechanism;
    if (mechanism == "SCRAM-SHA-256" || mechanism == "SCRAM-SHA-512")
    {
        return SendAndReceiveSASLSCRAMv0();
    }
    else if (mechanism == "GSSAPI")
    {
        return SendAndReceiveKerberos();
    }
    return SendAndReceiveSASLPlainAuthV0();
}

coev::awaitable<int> Broker::AuthenticateViaSASLv1()
{

    if (m_conf->Net.SASL.Handshake)
    {
        auto handshakeRequest = std::make_shared<SaslHandshakeRequest>();
        handshakeRequest->m_mechanism = m_conf->Net.SASL.Mechanism;
        handshakeRequest->m_version = m_conf->Net.SASL.Version;
        ResponsePromise<SaslHandshakeResponse> promise;
        int err = co_await SendInternal(handshakeRequest, promise);
        if (err)
        {
            co_return err;
        }

        if (promise.m_response->m_err != 0)
        {
            co_return promise.m_response->m_err;
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
        co_return co_await m_kerberos_authenticator->AuthorizeV2(
            shared_from_this(),
            [this](auto req, auto res) -> coev::awaitable<int>
            {
                ResponsePromise<SaslAuthenticateResponse> promise;
                auto err = co_await DefaultAuthSendReceiver(req, promise);
                if (!err)
                {
                    res = *promise.m_response;
                }
                co_return err;
            });
    }
    else if (mechanism == "OAUTHBEARER")
    {
        auto provider = m_conf->Net.SASL.TokenProvider_;
        co_return co_await SendAndReceiveSASLOAuth(
            provider,
            [this](auto req, auto res) -> coev::awaitable<int>
            {
                ResponsePromise<SaslAuthenticateResponse> promise;
                auto err = co_await DefaultAuthSendReceiver(req, promise);
                if (!err)
                {
                    res = *promise.m_response;
                }
                co_return err;
            });
    }
    else if (mechanism == "SCRAM-SHA-256" || mechanism == "SCRAM-SHA-512")
    {
        co_return co_await SendAndReceiveSASLSCRAMv1(
            m_conf->Net.SASL.ScramClientGeneratorFunc(),
            [this](auto req, auto res) -> coev::awaitable<int>
            {
                ResponsePromise<SaslAuthenticateResponse> promise;
                auto err = co_await DefaultAuthSendReceiver(req, promise);
                if (!err)
                {
                    res = *promise.m_response;
                }
                co_return err;
            });
    }
    co_return co_await SendAndReceiveSASLPlainAuthV1(
        [this](auto req, auto res) -> coev::awaitable<int>
        {
            ResponsePromise<SaslAuthenticateResponse> promise;
            auto err = co_await DefaultAuthSendReceiver(req, promise);
            if (!err)
            {
                res = *promise.m_response;
            }
            co_return err;
        });
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
    SaslHandshakeRequest rb;
    rb.m_mechanism = saslType;
    rb.m_version = version;

    Request req;
    req.m_correlation_id = m_correlation_id;
    req.m_client_id = m_conf->ClientID;
    req.m_body = static_cast<protocol_body *>(&rb);

    std::string buf;
    ::encode(req, buf);

    auto requestTime = std::chrono::system_clock::now();
    auto err = co_await Write(buf);
    if (err)
    {
        co_return err;
    }

    m_correlation_id++;
    std::string header;
    size_t headerBytes = 8;
    err = co_await ReadFull(header, headerBytes);
    if (err)
    {
        co_return err;
    }

    uint32_t length = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
    std::string payload;
    size_t n = length - 4;
    err = co_await ReadFull(payload, n);
    if (err)
    {
        co_return err;
    }

    SaslHandshakeResponse res;
    err = versioned_decode(payload, res, 0);
    if (err)
    {
        co_return err;
    }

    if (res.m_err != 0)
    {
        co_return res.m_err;
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
    memcpy(&authBytes[offset], m_conf->Net.SASL.AuthIdentity.c_str(), m_conf->Net.SASL.AuthIdentity.length());
    offset += m_conf->Net.SASL.AuthIdentity.length();
    authBytes[offset++] = 0;
    memcpy(&authBytes[offset], m_conf->Net.SASL.User.c_str(), m_conf->Net.SASL.User.length());
    offset += m_conf->Net.SASL.User.length();
    authBytes[offset++] = 0;
    memcpy(&authBytes[offset], m_conf->Net.SASL.Password.c_str(), m_conf->Net.SASL.Password.length());

    auto requestTime = std::chrono::system_clock::now();
    int32_t err = co_await Write(authBytes);
    if (err)
    {

        co_return err;
    }

    std::string header;
    size_t n = 4;
    err = co_await ReadFull(header, n);
    if (err)
    {
        co_return err;
    }
    co_return 0;
}

coev::awaitable<int>
Broker::SendAndReceiveSASLPlainAuthV1(AuthSendReceiver authSendReceiver)
{
    std::string authStr = m_conf->Net.SASL.AuthIdentity + "\x00" +
                          m_conf->Net.SASL.User + "\x00" +
                          m_conf->Net.SASL.Password;
    std::string authBytes(authStr.begin(), authStr.end());
    SaslAuthenticateResponse response;
    return authSendReceiver(authBytes, response);
}

int64_t Broker::CurrentUnixMilli()
{
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

coev::awaitable<int>
Broker::SendAndReceiveSASLOAuth(std::shared_ptr<AccessTokenProvider> provider,
                                AuthSendReceiver authSendReceiver)
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
    SaslAuthenticateResponse res;
    err = co_await authSendReceiver(message, res);
    if (err != ErrNoError)
    {
        co_return err;
    }

    bool isChallenge = !res.m_sasl_auth_bytes.empty();
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

        size_t length = msg.length();
        std::string authBytes;
        authBytes.resize(4);
        uint32_t len = length;
        authBytes[0] = (len >> 24) & 0xFF;
        authBytes[1] = (len >> 16) & 0xFF;
        authBytes[2] = (len >> 8) & 0xFF;
        authBytes[3] = len & 0xFF;
        err = co_await Write(authBytes);
        if (err)
        {
            co_return err;
        }

        err = co_await Write(msg);
        if (err)
        {
            co_return err;
        }

        m_correlation_id++;
        std::string header;
        size_t headerBytes = 4;
        err = co_await ReadFull(header, headerBytes);
        if (err)
        {
            co_return err;
        }

        uint32_t payloadLen = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
        std::string payload;
        size_t n = payloadLen;
        err = co_await ReadFull(payload, n);
        if (err)
        {
            co_return err;
        }

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
        SaslAuthenticateResponse res;
        err = co_await authSendReceiver(msg, res);
        if (err)
        {
            co_return err;
        }

        err = scramClient->Step(res.m_sasl_auth_bytes, msg);
        if (err)
        {
            co_return err;
        }
    }
    co_return 0;
}
coev::awaitable<int> Broker::SendAndReceiveApiVersions(int16_t v, ResponsePromise<ApiVersionsResponse> &promise)
{
    int err = 0;
    auto request = std::make_shared<ApiVersionsRequest>();
    request->m_version = v;
    err = co_await SendAndReceive(request, promise);
    if (err)
    {
        co_return err;
    }
    co_return 0;
}

int Broker::CreateSaslAuthenticateRequest(const std::string &msg, SaslAuthenticateRequest &request)
{
    request.m_sasl_auth_bytes = msg;
    if (m_conf->Version.IsAtLeast(V2_2_0_0))
    {
        request.m_version = 1;
    }
    return 0;
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
        ext = "\x01" + MapToString(token->m_extensions, "=", "\x01");
    }

    auto msg = "n,,\x01auth=Bearer " + token->m_token + ext + "\x01\x01";
    message.assign(msg.begin(), msg.end());
    return 0;
}

std::string Broker::MapToString(const std::map<std::string, std::string> &extensions, const std::string &keyValSep, const std::string &elemSep)
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

void Broker::ComputeSaslSessionLifetime(std::shared_ptr<SaslAuthenticateResponse> res)
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

        m_session_reauthentication_time = authenticationEndMs + sessionLifetimeMsToUse;
    }
    else
    {
        m_session_reauthentication_time = 0;
    }
}

void Broker::SafeAsyncClose()
{
    if (Connected())
    {
        auto err = Close();
        if (err != ErrNoError)
        {
            LOG_CORE("Error closing broker %d: %d", ID(), err);
        }
    }
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
