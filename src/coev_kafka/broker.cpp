
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
            LOG_CORE("conn is closed");
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
    if (m_conn->IsOpened() || m_conn->IsOpening())
    {
        return m_conn->Close();
    }
    return ErrNotConnected;
}

coev::awaitable<int> Broker::GetMetadata(std::shared_ptr<MetadataRequest> request, ResponsePromise<MetadataResponse> &response)
{
    return SendAndReceive(request, response);
}
coev::awaitable<int> Broker::GetConsumerMetadata(std::shared_ptr<ConsumerMetadataRequest> request, ResponsePromise<ConsumerMetadataResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::FindCoordinator(std::shared_ptr<FindCoordinatorRequest> request, ResponsePromise<FindCoordinatorResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::GetAvailableOffsets(std::shared_ptr<OffsetRequest> request, ResponsePromise<OffsetResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::Produce(std::shared_ptr<ProduceRequest> request, ResponsePromise<ProduceResponse> &response)
{
    if (request->m_acks == RequiredAcks::NoResponse)
    {
        return SendAndReceive(request, response);
    }
    else
    {
        return SendAndReceive(request, response);
    }
}

coev::awaitable<int> Broker::Fetch(std::shared_ptr<FetchRequest> request, ResponsePromise<FetchResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CommitOffset(std::shared_ptr<OffsetCommitRequest> request, ResponsePromise<OffsetCommitResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::FetchOffset(std::shared_ptr<OffsetFetchRequest> request, ResponsePromise<OffsetFetchResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::JoinGroup(std::shared_ptr<JoinGroupRequest> request, ResponsePromise<JoinGroupResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::SyncGroup(std::shared_ptr<SyncGroupRequest> request, ResponsePromise<SyncGroupResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::LeaveGroup(std::shared_ptr<LeaveGroupRequest> request, ResponsePromise<LeaveGroupResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::Heartbeat(std::shared_ptr<HeartbeatRequest> request, ResponsePromise<HeartbeatResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ListGroups(std::shared_ptr<ListGroupsRequest> request, ResponsePromise<ListGroupsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeGroups(std::shared_ptr<DescribeGroupsRequest> request, ResponsePromise<DescribeGroupsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ApiVersions(std::shared_ptr<ApiVersionsRequest> request, ResponsePromise<ApiVersionsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CreateTopics(std::shared_ptr<CreateTopicsRequest> request, ResponsePromise<CreateTopicsResponse> &response)
{

    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteTopics(std::shared_ptr<DeleteTopicsRequest> request, ResponsePromise<DeleteTopicsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CreatePartitions(std::shared_ptr<CreatePartitionsRequest> request, ResponsePromise<CreatePartitionsResponse> &response)
{

    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterPartitionReassignments(std::shared_ptr<AlterPartitionReassignmentsRequest> request, ResponsePromise<AlterPartitionReassignmentsResponse> &response)
{

    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ListPartitionReassignments(std::shared_ptr<ListPartitionReassignmentsRequest> request, ResponsePromise<ListPartitionReassignmentsResponse> &response)
{

    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ElectLeaders(std::shared_ptr<ElectLeadersRequest> request, ResponsePromise<ElectLeadersResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteRecords(std::shared_ptr<DeleteRecordsRequest> request, ResponsePromise<DeleteRecordsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeAcls(std::shared_ptr<DescribeAclsRequest> request, ResponsePromise<DescribeAclsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CreateAcls(std::shared_ptr<CreateAclsRequest> request, ResponsePromise<CreateAclsResponse> &response)
{
    int32_t err = co_await SendAndReceive(request, response);
    if (err)
    {
        co_return err;
    }
    std::vector<int> errs;
    for (auto &res : response.m_response->m_acl_creation_responses)
    {
        if (res.m_err != ErrNoError)
        {
            errs.push_back(res.m_err);
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
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::InitProducerID(std::shared_ptr<InitProducerIDRequest> request, ResponsePromise<InitProducerIDResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AddPartitionsToTxn(std::shared_ptr<AddPartitionsToTxnRequest> request, ResponsePromise<AddPartitionsToTxnResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AddOffsetsToTxn(std::shared_ptr<AddOffsetsToTxnRequest> request, ResponsePromise<AddOffsetsToTxnResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::EndTxn(std::shared_ptr<EndTxnRequest> request, ResponsePromise<EndTxnResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::TxnOffsetCommit(std::shared_ptr<TxnOffsetCommitRequest> request, ResponsePromise<TxnOffsetCommitResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeConfigs(std::shared_ptr<DescribeConfigsRequest> request, ResponsePromise<DescribeConfigsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterConfigs(std::shared_ptr<AlterConfigsRequest> request, ResponsePromise<AlterConfigsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::IncrementalAlterConfigs(std::shared_ptr<IncrementalAlterConfigsRequest> request, ResponsePromise<IncrementalAlterConfigsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteGroups(std::shared_ptr<DeleteGroupsRequest> request, ResponsePromise<DeleteGroupsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteOffsets(std::shared_ptr<DeleteOffsetsRequest> request, ResponsePromise<DeleteOffsetsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeLogDirs(std::shared_ptr<DescribeLogDirsRequest> request, ResponsePromise<DescribeLogDirsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeUserScramCredentials(std::shared_ptr<DescribeUserScramCredentialsRequest> req, ResponsePromise<DescribeUserScramCredentialsResponse> &res)
{
    return SendAndReceive(req, res);
}

coev::awaitable<int> Broker::AlterUserScramCredentials(std::shared_ptr<AlterUserScramCredentialsRequest> req, ResponsePromise<AlterUserScramCredentialsResponse> &res)
{
    return SendAndReceive(req, res);
}

coev::awaitable<int> Broker::DescribeClientQuotas(std::shared_ptr<DescribeClientQuotasRequest> request, ResponsePromise<DescribeClientQuotasResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterClientQuotas(std::shared_ptr<AlterClientQuotasRequest> request, ResponsePromise<AlterClientQuotasResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::Ready()
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
    co_return 0;
}
coev::awaitable<int> Broker::ReadFull(std::string &buf, size_t n)
{
    assert(m_conn->IsOpened());
    buf.resize(n);
    int err = co_await m_conn->ReadFull(buf, n);
    if (err != ErrNoError)
    {
        co_return INVALID;
    }
    co_return 0;
}

coev::awaitable<int> Broker::Write(const std::string &buf)
{
    assert(m_conn->IsOpened());
    return m_conn->Write(buf);
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
        auto request = std::make_shared<SaslHandshakeRequest>();
        request->m_mechanism = m_conf->Net.SASL.Mechanism;
        request->m_version = m_conf->Net.SASL.Version;
        ResponsePromise<SaslHandshakeResponse> promise;
        int err = co_await SendInternal(request, promise);
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
                    res = promise.m_response;
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
                    res = promise.m_response;
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
                    res = promise.m_response;
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
                res = promise.m_response;
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
    return m_kerberos_authenticator->Authorize(shared_from_this());
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

    auto err = co_await Write(buf);
    if (err)
    {
        LOG_CORE("Write %d %d %s", err, errno, strerror(errno));
        co_return err;
    }

    co_await RLock();
    defer(RUnlock());

    m_correlation_id++;
    std::string header;
    size_t headerBytes = 8;
    err = co_await ReadFull(header, headerBytes);
    if (err)
    {
        LOG_CORE("Read %d %d %s", err, errno, strerror(errno));
        co_return err;
    }

    uint32_t length = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
    std::string payload;
    size_t n = length - 4;
    err = co_await ReadFull(payload, n);
    if (err)
    {
        LOG_CORE("Read %d %d %s", err, errno, strerror(errno));
        co_return err;
    }

    SaslHandshakeResponse res;
    err = decode_version(payload, res, 0);
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
    std::string auth_bytes;
    auth_bytes.resize(length + 4);
    uint32_t len = length;
    auth_bytes[0] = (len >> 24) & 0xFF;
    auth_bytes[1] = (len >> 16) & 0xFF;
    auth_bytes[2] = (len >> 8) & 0xFF;
    auth_bytes[3] = len & 0xFF;

    size_t offset = 4;
    memcpy(&auth_bytes[offset], m_conf->Net.SASL.AuthIdentity.c_str(), m_conf->Net.SASL.AuthIdentity.length());
    offset += m_conf->Net.SASL.AuthIdentity.length();
    auth_bytes[offset++] = 0;
    memcpy(&auth_bytes[offset], m_conf->Net.SASL.User.c_str(), m_conf->Net.SASL.User.length());
    offset += m_conf->Net.SASL.User.length();
    auth_bytes[offset++] = 0;
    memcpy(&auth_bytes[offset], m_conf->Net.SASL.Password.c_str(), m_conf->Net.SASL.Password.length());

    int32_t err = co_await Write(auth_bytes);
    if (err)
    {
        LOG_CORE("Write %d %d %s", err, errno, strerror(errno));
        co_return err;
    }

    std::string header;
    size_t n = 4;
    err = co_await ReadFull(header, n);
    if (err)
    {
        LOG_CORE("Read %d %d %s", err, errno, strerror(errno));
        co_return err;
    }
    co_return 0;
}

coev::awaitable<int> Broker::SendAndReceiveSASLPlainAuthV1(AuthSendReceiver authSendReceiver)
{
    std::string authStr = m_conf->Net.SASL.AuthIdentity + "\x00" +
                          m_conf->Net.SASL.User + "\x00" +
                          m_conf->Net.SASL.Password;
    std::string auth_bytes(authStr.begin(), authStr.end());
    std::shared_ptr<SaslAuthenticateResponse> response = std::make_shared<SaslAuthenticateResponse>();
    return authSendReceiver(auth_bytes, response);
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
    std::shared_ptr<SaslAuthenticateResponse> res = std::make_shared<SaslAuthenticateResponse>();
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

    auto scram_client = m_conf->Net.SASL.ScramClientGeneratorFunc();
    err = scram_client->Begin(m_conf->Net.SASL.User, m_conf->Net.SASL.Password, m_conf->Net.SASL.ScramAuthzID);
    if (err)
    {
        co_return 1;
    }

    std::string msg;
    err = scram_client->Step("", msg);
    if (err)
    {
        co_return 1;
    }

    while (!scram_client->Done())
    {
        size_t length = msg.length();
        std::string auth_bytes;
        auth_bytes.resize(4);
        uint32_t len = length;
        auth_bytes[0] = (len >> 24) & 0xFF;
        auth_bytes[1] = (len >> 16) & 0xFF;
        auth_bytes[2] = (len >> 8) & 0xFF;
        auth_bytes[3] = len & 0xFF;
        {
            co_await WLock();
            defer(WUnlock());
            err = co_await Write(auth_bytes);
            if (err)
            {
                co_return err;
            }

            err = co_await Write(msg);
            if (err)
            {
                co_return err;
            }
        }

        co_await RLock();
        defer(RUnlock());

        m_correlation_id++;
        std::string header;
        size_t header_bytes = 4;
        err = co_await ReadFull(header, header_bytes);
        if (err)
        {
            LOG_CORE("Read %d %d %s", err, errno, strerror(errno));
            co_return err;
        }

        uint32_t payload_len = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
        std::string payload;
        err = co_await ReadFull(payload, payload_len);
        if (err)
        {
            LOG_CORE("Read %d %d %s", err, errno, strerror(errno));
            co_return err;
        }

        err = scram_client->Step(payload, msg);
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

    std::shared_ptr<SaslAuthenticateResponse> res = std::make_shared<SaslAuthenticateResponse>();
    while (!scramClient->Done())
    {
        err = co_await authSendReceiver(msg, res);
        if (err)
        {
            co_return err;
        }

        err = scramClient->Step(res->m_sasl_auth_bytes, msg);
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
    auto request = std::make_shared<ApiVersionsRequest>(v);
    err = co_await SendAndReceive(request, promise);
    if (err)
    {
        co_return err;
    }
    co_return 0;
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
    message = "n,,\x01auth=Bearer " + token->m_token + ext + "\x01\x01";
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
