#include "broker.h"
#include "access_token.h"
#include "kerberos_client.h"
#include "response_header.h"
#include "scram_client.h"
#include "sleep_for.h"
#include "undefined.h"
#include <chrono>
#include <coev/coev.h>
#include <cstring>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>

int8_t getHeaderLength(int16_t header_version)
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

Broker::Broker(const std::string &addr) : m_id(-1), m_addr(addr), m_correlation_id(0), m_session_reauthentication_time(0), m_task()
{
}
Broker::Broker(int id, const std::string &addr) : m_id(id), m_addr(addr), m_correlation_id(0), m_session_reauthentication_time(0), m_task()
{
}
Broker::~Broker() { LOG_CORE("broker %p closed", this); }
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
    if (m_conn.IsOpened())
    {
        LOG_CORE("conn is opened");
        co_return INVALID;
    }

    if (m_conn.IsOpening())
    {
        LOG_CORE("conn is not opened");
        co_await m_opened.suspend();
        co_return 0;
    }

    auto [host, port] = net::SplitHostPort(m_addr);
    LOG_CORE("connect to %s:%d", host.data(), port);
    auto fd = co_await m_conn.Dial(host.data(), port);
    if (fd == INVALID)
    {
        LOG_CORE("connect to %s:%d failed", host.data(), port);
        co_return INVALID;
    }

    if (m_id >= 0)
    {
    }
    LOG_CORE("connected %s:%d", host.data(), port);
    int err = ErrNoError;
    if (m_conf->ApiVersionsRequest)
    {
        ResponsePromise<ApiVersionsResponse> apiVersionsResponse;
        err = co_await SendAndReceiveApiVersions(3, apiVersionsResponse);
        if (err)
        {
            // 如果第一次调用失败，尝试使用版本0重新调用
            err = co_await SendAndReceiveApiVersions(0, apiVersionsResponse);
            if (err)
            {
                // 如果第二次调用也失败，关闭连接并返回错误
                m_conn.Close();
                LOG_CORE("connect to %s:%d failed", host.data(), port);
                co_return INVALID;
            }
        }
        m_broker_api_versions.clear();
        for (auto &key : apiVersionsResponse.m_response.m_api_keys)
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
            m_conn.Close();
            LOG_CORE("connect to %s:%d failed", host.data(), port);
            co_return INVALID;
        }
    }

    if (m_conf->Net.SASL.Enable && !useSaslV0)
    {
        err = co_await AuthenticateViaSASLv1();
        if (err)
        {
            co_await m_done.get();
            m_conn.Close();
            LOG_CORE("connect to %s:%d failed", host.data(), port)
            co_return INVALID;
        }
    }

    co_return 0;
}

bool Broker::Connected()
{
    return m_conn.IsOpened();
}

int Broker::TLSConnectionState()
{
    if (!m_conn.IsOpened())
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

coev::awaitable<int> Broker::Close()
{
    if (!m_conn.IsOpened())
    {
        co_return ErrNotConnected;
    }

    co_await m_done.get();
    int32_t err = m_conn.Close();
    co_return 0;
}

int32_t Broker::ID() { return m_id; }

std::string Broker::Addr() { return m_addr; }

std::string Broker::Rack() { return m_rack; }

coev::awaitable<int> Broker::GetMetadata(const MetadataRequest &request, ResponsePromise<MetadataResponse> &response)
{
    return SendAndReceive(request, response);
}
coev::awaitable<int> Broker::GetConsumerMetadata(const ConsumerMetadataRequest &request, ResponsePromise<ConsumerMetadataResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::FindCoordinator(const FindCoordinatorRequest &request, ResponsePromise<FindCoordinatorResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::GetAvailableOffsets(const OffsetRequest &request, ResponsePromise<OffsetResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AsyncProduce(const ProduceRequest &request, ResponsePromise<ProduceResponse> &response)
{
    bool needAcks = request.m_acks != NoResponse;
    if (needAcks)
    {
    }
    auto err = co_await SendAndReceive(request, response);
    co_return err;
}

coev::awaitable<int> Broker::Produce(const ProduceRequest &request, ResponsePromise<ProduceResponse> &response)
{
    if (request.m_acks == RequiredAcks::NoResponse)
    {
        int32_t err = co_await SendAndReceive(request, response);
        co_return err;
    }
    else
    {
        co_return co_await SendAndReceive(request, response);
    }
}

coev::awaitable<int> Broker::Fetch(const FetchRequest &request, ResponsePromise<FetchResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CommitOffset(const OffsetCommitRequest &request, ResponsePromise<OffsetCommitResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::FetchOffset(const OffsetFetchRequest &request, ResponsePromise<OffsetFetchResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::JoinGroup(const JoinGroupRequest &request, ResponsePromise<JoinGroupResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::SyncGroup(const SyncGroupRequest &request, ResponsePromise<SyncGroupResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::LeaveGroup(const LeaveGroupRequest &request, ResponsePromise<LeaveGroupResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::Heartbeat(const HeartbeatRequest &request, ResponsePromise<HeartbeatResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ListGroups(const ListGroupsRequest &request, ResponsePromise<ListGroupsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeGroups(const DescribeGroupsRequest &request, ResponsePromise<DescribeGroupsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ApiVersions(const ApiVersionsRequest &request, ResponsePromise<ApiVersionsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CreateTopics(const CreateTopicsRequest &request, ResponsePromise<CreateTopicsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteTopics(const DeleteTopicsRequest &request, ResponsePromise<DeleteTopicsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CreatePartitions(const CreatePartitionsRequest &request, ResponsePromise<CreatePartitionsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterPartitionReassignments(const AlterPartitionReassignmentsRequest &request, ResponsePromise<AlterPartitionReassignmentsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ListPartitionReassignments(const ListPartitionReassignmentsRequest &request, ResponsePromise<ListPartitionReassignmentsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ElectLeaders(const ElectLeadersRequest &request, ResponsePromise<ElectLeadersResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteRecords(const DeleteRecordsRequest &request, ResponsePromise<DeleteRecordsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeAcls(const DescribeAclsRequest &request, ResponsePromise<DescribeAclsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::CreateAcls(const CreateAclsRequest &request, ResponsePromise<CreateAclsResponse> &response)
{
    int32_t err = co_await SendAndReceive(request, response);
    if (err)
    {
        co_return err;
    }
    std::vector<int> errs;
    for (auto &res : response.m_response.m_acl_creation_responses)
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

coev::awaitable<int> Broker::DeleteAcls(const DeleteAclsRequest &request, ResponsePromise<DeleteAclsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::InitProducerID(const InitProducerIDRequest &request, ResponsePromise<InitProducerIDResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AddPartitionsToTxn(const AddPartitionsToTxnRequest &request, ResponsePromise<AddPartitionsToTxnResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AddOffsetsToTxn(const AddOffsetsToTxnRequest &request, ResponsePromise<AddOffsetsToTxnResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::EndTxn(const EndTxnRequest &request, ResponsePromise<EndTxnResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::TxnOffsetCommit(const TxnOffsetCommitRequest &request, ResponsePromise<TxnOffsetCommitResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeConfigs(const DescribeConfigsRequest &request, ResponsePromise<DescribeConfigsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterConfigs(const AlterConfigsRequest &request, ResponsePromise<AlterConfigsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::IncrementalAlterConfigs(const IncrementalAlterConfigsRequest &request, ResponsePromise<IncrementalAlterConfigsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteGroups(const DeleteGroupsRequest &request, ResponsePromise<DeleteGroupsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DeleteOffsets(const DeleteOffsetsRequest &request, ResponsePromise<DeleteOffsetsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeLogDirs(const DescribeLogDirsRequest &request, ResponsePromise<DescribeLogDirsResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::DescribeUserScramCredentials(const DescribeUserScramCredentialsRequest &req, ResponsePromise<DescribeUserScramCredentialsResponse> &res)
{
    return SendAndReceive(req, res);
}

coev::awaitable<int> Broker::AlterUserScramCredentials(const AlterUserScramCredentialsRequest &req, ResponsePromise<AlterUserScramCredentialsResponse> &res)
{
    return SendAndReceive(req, res);
}

coev::awaitable<int> Broker::DescribeClientQuotas(const DescribeClientQuotasRequest &request, ResponsePromise<DescribeClientQuotasResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::AlterClientQuotas(const AlterClientQuotasRequest &request, ResponsePromise<AlterClientQuotasResponse> &response)
{
    return SendAndReceive(request, response);
}

coev::awaitable<int> Broker::ReadFull(std::string &buf, size_t n)
{
    if (m_conn.IsClosed())
    {
        co_return INVALID;
    }
    if (m_conn.IsOpening())
    {
        co_await m_opened.suspend();
        if (m_conn.IsClosed())
        {
            co_return INVALID;
        }
    }
    if (buf.size() < n)
    {
        buf.resize(n);
    }
    int err = ErrNoError;
    co_task task;
    auto _timeout = task << [](auto _this) -> coev::awaitable<void>
    {
        co_await sleep_for(_this->m_conf->Net.ReadTimeout);
    }(shared_from_this());
    auto _send = task << [](auto _this, auto &buf, auto n, int &err) -> coev::awaitable<void>
    {
        err = co_await _this->m_conn.ReadFull(buf, n);
        if (err != ErrNoError)
        {
            LOG_ERR("ReadFull %d %d %s", err, errno, strerror(errno));
        }
    }(shared_from_this(), buf, n, err);
    auto _result = co_await task.wait();
    if (_result == _timeout)
    {
        co_return ErrTimedOut;
    }
    if (err)
    {
        co_return err;
    }
    co_return 0;
}

coev::awaitable<int> Broker::Write(const std::string &buf)
{

    auto now = std::chrono::system_clock::now();
    if (m_conn.IsClosed())
    {
        co_return INVALID;
    }
    if (m_conn.IsOpening())
    {
        co_await m_opened.suspend();
        if (m_conn.IsClosed())
        {
            co_return ErrNotConnected;
        }
    }
    co_return co_await m_conn.Write(buf);
}

int Broker::decode(packetDecoder &pd, int16_t version)
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

    int32_t emptyArray;
    err = pd.getEmptyTaggedFieldArray(emptyArray);
    return err;
}

int Broker::encode(packetEncoder &pe, int16_t version)
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
    else
    {
        return SendAndReceiveSASLPlainAuthV0();
    }
}

coev::awaitable<int> Broker::AuthenticateViaSASLv1()
{

    if (m_conf->Net.SASL.Handshake)
    {
        SaslHandshakeRequest handshakeRequest;
        handshakeRequest.m_mechanism = m_conf->Net.SASL.Mechanism;
        handshakeRequest.m_version = m_conf->Net.SASL.Version;
        ResponsePromise<SaslHandshakeResponse> promise;
        int err = co_await SendInternal(handshakeRequest, promise);
        if (err)
        {
            co_return err;
        }

        if (promise.m_response.m_err != 0)
        {
            co_return promise.m_response.m_err;
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
            shared_from_this(), [this](auto req, auto res) -> coev::awaitable<int>
            { 
                ResponsePromise<SaslAuthenticateResponse> promise;
                auto err = co_await DefaultAuthSendReceiver(req, promise);
                if (!err) {
                    res = promise.m_response;
                }
                co_return err; });
    }
    else if (mechanism == "OAUTHBEARER")
    {
        auto provider = m_conf->Net.SASL.TokenProvider_;
        co_return co_await SendAndReceiveSASLOAuth(
            provider, [this](auto req, auto res) -> coev::awaitable<int>
            { 
                ResponsePromise<SaslAuthenticateResponse> promise;
                auto err = co_await DefaultAuthSendReceiver(req, promise);
                if (!err) {
                    res = promise.m_response;
                }
                co_return err; });
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
    else
    {
        co_return co_await SendAndReceiveSASLPlainAuthV1(
            [this](auto req, auto res) -> coev::awaitable<int>
            { 
                                                             ResponsePromise<SaslAuthenticateResponse> promise;
                                                             auto err = co_await DefaultAuthSendReceiver(req, promise);
                                                             if (!err) {
                                                                 res = promise.m_response;
                                                             }
                                                             co_return err; });
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
    SaslHandshakeRequest rb;
    rb.m_mechanism = saslType;
    rb.m_version = version;

    Request req;
    req.m_correlation_id = m_correlation_id;
    req.m_client_id = m_conf->ClientID;
    req.m_body = &rb;

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
    err = versionedDecode(payload, res, 0);
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
        int32_t handshakeErr =
            co_await SendAndReceiveSASLHandshake("PLAIN", m_conf->Net.SASL.Version);
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
    ApiVersionsRequest request;
    request.m_version = v;
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
    m_task << [](auto b) -> coev::awaitable<void>
    {
        if (!b->Connected())
        {
            auto err = co_await b->Close();
            if (err != ErrNoError)
            {
                LOG_CORE("Error closing broker %d: %d", b->ID(), err);
            }
        }
        co_return;
    }(shared_from_this());
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
