#pragma once

#include <atomic>
#include <chrono>
#include <coev/coev.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "client.h"
#include "config.h"
#include "connect.h"

#include "access_token.h"
#include "acl_create_request.h"
#include "acl_create_response.h"
#include "acl_delete_request.h"
#include "acl_delete_response.h"
#include "acl_describe_request.h"
#include "acl_describe_response.h"
#include "add_offsets_to_txn_request.h"
#include "add_offsets_to_txn_response.h"
#include "add_partitions_to_txn_request.h"
#include "add_partitions_to_txn_response.h"
#include "alter_client_quotas_request.h"
#include "alter_client_quotas_response.h"
#include "alter_configs_request.h"
#include "alter_configs_response.h"
#include "alter_partition_reassignments_request.h"
#include "alter_partition_reassignments_response.h"
#include "alter_user_scram_credentials_request.h"
#include "alter_user_scram_credentials_response.h"
#include "api_versions.h"
#include "api_versions_request.h"
#include "api_versions_response.h"
#include "connect.h"
#include "consumer_metadata_request.h"
#include "consumer_metadata_response.h"
#include "create_partitions_request.h"
#include "create_partitions_response.h"
#include "create_topics_request.h"
#include "create_topics_response.h"
#include "delete_groups_request.h"
#include "delete_groups_response.h"
#include "delete_offsets_request.h"
#include "delete_offsets_response.h"
#include "delete_records_request.h"
#include "delete_records_response.h"
#include "delete_topics_request.h"
#include "delete_topics_response.h"
#include "describe_client_quotas_request.h"
#include "describe_client_quotas_response.h"
#include "describe_configs_request.h"
#include "describe_configs_response.h"
#include "describe_groups_request.h"
#include "describe_groups_response.h"
#include "describe_log_dirs_request.h"
#include "describe_log_dirs_response.h"
#include "describe_user_scram_credentials_request.h"
#include "describe_user_scram_credentials_response.h"
#include "elect_leaders_request.h"
#include "elect_leaders_response.h"
#include "end_txn_request.h"
#include "end_txn_response.h"
#include "fetch_request.h"
#include "fetch_response.h"
#include "find_coordinator_request.h"
#include "find_coordinator_response.h"
#include "gssapi_kerberos.h"
#include "heartbeat_request.h"
#include "heartbeat_response.h"
#include "incremental_alter_configs_request.h"
#include "incremental_alter_configs_response.h"
#include "init_producer_id_request.h"
#include "init_producer_id_response.h"
#include "join_group_request.h"
#include "join_group_response.h"
#include "leave_group_request.h"
#include "leave_group_response.h"
#include "list_groups_request.h"
#include "list_groups_response.h"
#include "list_partition_reassignments_request.h"
#include "list_partition_reassignments_response.h"
#include "metadata_request.h"
#include "metadata_response.h"
#include "offset_commit_request.h"
#include "offset_commit_response.h"
#include "offset_fetch_request.h"
#include "offset_fetch_response.h"
#include "offset_request.h"
#include "offset_response.h"
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "produce_request.h"
#include "produce_response.h"
#include "request.h"
#include "response_promise.h"
#include "response_header.h"
#include "sasl_authenticate_request.h"
#include "sasl_authenticate_response.h"
#include "sasl_handshake_request.h"
#include "sasl_handshake_response.h"
#include "sync_group_request.h"
#include "sync_group_response.h"
#include "tls.h"
#include "txn_offset_commit_request.h"
#include "txn_offset_commit_response.h"
#include "undefined.h"

// Forward declaration
int8_t getHeaderLength(int16_t header_version);

struct GSSAPIKerberosAuth;

struct Broker : versioned_encoder, versioned_decoder, std::enable_shared_from_this<Broker>
{

    Broker() = default;
    Broker(const std::string &addr);
    Broker(int id, const std::string &addr);
    ~Broker();
    coev::awaitable<int> Open(std::shared_ptr<Config> conf);
    int ResponseSize();
    bool Connected();
    int TLSConnectionState();
    int32_t ID();
    std::string Addr();
    std::string Rack();

    coev::awaitable<int> Close();
    coev::awaitable<int> GetMetadata(const MetadataRequest &request, MetadataResponse &response);
    coev::awaitable<int> GetConsumerMetadata(const ConsumerMetadataRequest &request, ConsumerMetadataResponse &response);
    coev::awaitable<int> FindCoordinator(const FindCoordinatorRequest &request, FindCoordinatorResponse &response);
    coev::awaitable<int> GetAvailableOffsets(const OffsetRequest &request, OffsetResponse &response);
    coev::awaitable<int> Produce(const ProduceRequest &request, ProduceResponse &response);
    coev::awaitable<int> Fetch(const FetchRequest &request, FetchResponse &response);
    coev::awaitable<int> CommitOffset(const OffsetCommitRequest &request, OffsetCommitResponse &response);
    coev::awaitable<int> FetchOffset(const OffsetFetchRequest &request, OffsetFetchResponse &response);
    coev::awaitable<int> JoinGroup(const JoinGroupRequest &request, JoinGroupResponse &response);
    coev::awaitable<int> SyncGroup(const SyncGroupRequest &request, SyncGroupResponse &response);
    coev::awaitable<int> LeaveGroup(const LeaveGroupRequest &request, LeaveGroupResponse &response);
    coev::awaitable<int> Heartbeat(const HeartbeatRequest &request, HeartbeatResponse &response);
    coev::awaitable<int> ListGroups(const ListGroupsRequest &request, ListGroupsResponse &response);
    coev::awaitable<int> DescribeGroups(const DescribeGroupsRequest &request, DescribeGroupsResponse &response);
    coev::awaitable<int> ApiVersions(const ApiVersionsRequest &request, ApiVersionsResponse &response);
    coev::awaitable<int> CreateTopics(const CreateTopicsRequest &request, CreateTopicsResponse &response);
    coev::awaitable<int> DeleteTopics(const DeleteTopicsRequest &request, DeleteTopicsResponse &response);
    coev::awaitable<int> CreatePartitions(const CreatePartitionsRequest &request, CreatePartitionsResponse &response);
    coev::awaitable<int> AlterPartitionReassignments(const AlterPartitionReassignmentsRequest &request, AlterPartitionReassignmentsResponse &response);
    coev::awaitable<int> ListPartitionReassignments(const ListPartitionReassignmentsRequest &request, ListPartitionReassignmentsResponse &response);
    coev::awaitable<int> ElectLeaders(const ElectLeadersRequest &request, ElectLeadersResponse &response);
    coev::awaitable<int> DeleteRecords(const DeleteRecordsRequest &request, DeleteRecordsResponse &response);
    coev::awaitable<int> DescribeAcls(const DescribeAclsRequest &request, DescribeAclsResponse &response);
    coev::awaitable<int> CreateAcls(const CreateAclsRequest &request, CreateAclsResponse &response);
    coev::awaitable<int> DeleteAcls(const DeleteAclsRequest &request, DeleteAclsResponse &response);
    coev::awaitable<int> InitProducerID(const InitProducerIDRequest &request, InitProducerIDResponse &response);
    coev::awaitable<int> AddPartitionsToTxn(const AddPartitionsToTxnRequest &request, AddPartitionsToTxnResponse &response);
    coev::awaitable<int> AddOffsetsToTxn(const AddOffsetsToTxnRequest &request, AddOffsetsToTxnResponse &response);
    coev::awaitable<int> EndTxn(const EndTxnRequest &request, EndTxnResponse &response);
    coev::awaitable<int> TxnOffsetCommit(const TxnOffsetCommitRequest &request, TxnOffsetCommitResponse &response);
    coev::awaitable<int> DescribeConfigs(const DescribeConfigsRequest &request, DescribeConfigsResponse &response);
    coev::awaitable<int> AlterConfigs(const AlterConfigsRequest &request, AlterConfigsResponse &response);
    coev::awaitable<int> IncrementalAlterConfigs(const IncrementalAlterConfigsRequest &request, IncrementalAlterConfigsResponse &response);
    coev::awaitable<int> DeleteGroups(const DeleteGroupsRequest &request, DeleteGroupsResponse &response);
    coev::awaitable<int> DeleteOffsets(const DeleteOffsetsRequest &request, DeleteOffsetsResponse &response);
    coev::awaitable<int> DescribeLogDirs(const DescribeLogDirsRequest &request, DescribeLogDirsResponse &response);
    coev::awaitable<int> DescribeUserScramCredentials(const DescribeUserScramCredentialsRequest &req, DescribeUserScramCredentialsResponse &res);
    coev::awaitable<int> AlterUserScramCredentials(const AlterUserScramCredentialsRequest &req, AlterUserScramCredentialsResponse &res);
    coev::awaitable<int> DescribeClientQuotas(const DescribeClientQuotasRequest &request, DescribeClientQuotasResponse &response);
    coev::awaitable<int> AlterClientQuotas(const AlterClientQuotasRequest &request, AlterClientQuotasResponse &response);
    coev::awaitable<int> AsyncProduce(const ProduceRequest &request, std::function<void(ProduceResponse &, KError)> callback);

    std::shared_ptr<Config> m_conf;
    Connect m_conn;
    std::string m_rack;
    int32_t m_id;
    std::string m_addr;
    int32_t m_correlation_id;
    coev::async m_opened;

    ApiVersionMap m_broker_api_versions;
    std::shared_ptr<GSSAPIKerberosAuth> m_kerberos_authenticator;
    int64_t m_session_reauthentication_time;

    std::mutex m_throttle_timer_lock;
    coev::co_task m_task;
    coev::co_channel<bool> m_done;

    coev::awaitable<int> ReadFull(std::string &buf, size_t n);
    coev::awaitable<int> Write(const std::string &buf);

    coev::awaitable<int> AuthenticateViaSASLv0();
    coev::awaitable<int> AuthenticateViaSASLv1();
    coev::awaitable<int> SendAndReceiveKerberos();
    coev::awaitable<int> SendAndReceiveSASLHandshake(const std::string &saslType,
                                                     int16_t version);
    coev::awaitable<int> SendAndReceiveSASLPlainAuthV0();
    coev::awaitable<int> SendAndReceiveSASLPlainAuthV1(AuthSendReceiver authSendReceiver);
    coev::awaitable<int> SendAndReceiveSASLOAuth(std::shared_ptr<AccessTokenProvider> provider, AuthSendReceiver authSendReceiver);
    coev::awaitable<int> SendAndReceiveSASLSCRAMv0();
    coev::awaitable<int> SendAndReceiveSASLSCRAMv1(std::shared_ptr<SCRAMClient> scramClient, AuthSendReceiver authSendReceiver);
    SaslAuthenticateRequest CreateSaslAuthenticateRequest(const std::string &msg);

    int decode(packetDecoder &pd, int16_t version);
    int encode(packetEncoder &pe, int16_t version);
    void SafeAsyncClose();

    int BuildClientFirstMessage(std::shared_ptr<AccessToken> token, std::string &);
    std::string MapToString(const std::map<std::string, std::string> &extensions, const std::string &keyValSep, const std::string &elemSep);
    int64_t CurrentUnixMilli();
    void ComputeSaslSessionLifetime(std::shared_ptr<SaslAuthenticateResponse> res);

    template <class Req, class Resp>
    coev::awaitable<int> Send(Req &request, Resp &response, ResponsePromise<Resp> &promise)
    {
        return SendWithPromise(request, promise);
    }
    template <class Req, class Resp>
    coev::awaitable<int> Send(Req &req, ResponsePromise<Resp> &promise)
    {
        return SendWithPromise(req, promise);
    }

    template <class Req, class Resp>
    coev::awaitable<int> SendWithPromise(Req &request, ResponsePromise<Resp> &promise)
    {
        int err = 0;
        if (m_session_reauthentication_time > 0 &&
            CurrentUnixMilli() > m_session_reauthentication_time)
        {
            err = co_await AuthenticateViaSASLv1();
            if (err)
            {
                co_return err;
            }
        }
        err = co_await SendInternal(request, promise);
        co_return err;
    }
    template <class Req, class Resp>
    coev::awaitable<int> SendOffsetCommit(const Req &req, Resp &resp, ResponsePromise<Resp> &promise)
    {
        return Send(req, resp, promise);
    }
    template <class Req, class Resp>
    coev::awaitable<int> SendAndReceive(const Req &req, Resp &res)
    {
        ResponsePromise<Resp> promise;
        Req non_const_req = req;
        int32_t err = co_await Send(non_const_req, res, promise);
        if (err)
        {
            co_return err;
        }

        err = co_await HandleResponsePromise(req, res, promise);
        if (err)
        {
            co_return err;
        }
        co_return 0;
    }

    template <class Req, class Resp>
    coev::awaitable<int> SendInternal(Req &rb, ResponsePromise<Resp> &promise)
    {
        if (!m_conn)
        {
            co_await m_opened.suspend();
            if (!m_conn)
            {
                co_return ErrNotConnected;
            }
        }

        int16_t version;
        KafkaVersion required_version;

        using NonConstReq = std::remove_const_t<Req>;
        NonConstReq non_const_rb = rb;

        version = non_const_rb.version();
        required_version = non_const_rb.required_version();
        restrictApiVersion(non_const_rb, m_broker_api_versions);
        request request;
        request.m_correlation_id = m_correlation_id;
        request.m_client_id = m_conf->ClientID;
        request.m_body = std::make_shared<NonConstReq>(non_const_rb);
        std::string buf;
        ::encode(request, buf);

        auto requestTime = std::chrono::system_clock::now();

        auto err = co_await Write(buf);
        if (err)
        {
            LOG_CORE("Write %d %s", errno, strerror(errno));
            co_return err;
        }

        m_correlation_id++;
        // Always process response for value types

        promise.m_request_time = requestTime;
        promise.m_correlation_id = request.m_correlation_id;
        co_await ResponseReceiver(promise);
        co_return 0;
    }

    template <class Resp>
    coev::awaitable<int> SendAndReceiveApiVersions(int16_t v, Resp &response)
    {
        ApiVersionsRequest request;
        request.m_version = v;
        ApiVersionsResponse response_obj;
        ResponsePromise<ApiVersionsResponse> promise;
        auto err = co_await Send(request, response_obj, promise);
        if (err)
        {
            co_return err;
        }
        // Only handle non-shared_ptr case
        response = response_obj;
        co_return 0;
    }
    template <class Resp>
    coev::awaitable<int> ResponseReceiver(ResponsePromise<Resp> &promise)
    {

        std::string header;
        auto bytesReadHeader = getHeaderLength(promise.m_response.header_version());
        auto err = co_await ReadFull(header, bytesReadHeader);
        auto m_request_latency = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - promise.m_request_time);
        if (err)
        {
            std::string _;
            promise.Handle(_, KError(err));
            co_return INVALID;
        }

        responseHeader decodedHeader;
        err = versionedDecode(header, decodedHeader, promise.m_response.header_version());
        if (err)
        {
            std::string _;
            promise.Handle(_, KError(err));
            co_return INVALID;
        }

        if (decodedHeader.m_correlation_id != promise.m_correlation_id)
        {
            std::string _;
            promise.Handle(_, KError(1));
            co_return INVALID;
        }

        std::string buf;
        size_t bytesReadBody = decodedHeader.m_length - bytesReadHeader + 4;
        err = co_await ReadFull(buf, bytesReadBody);
        if (err)
        {

            std::string _;
            promise.Handle(_, KError(err));
            co_return INVALID;
        }
        promise.Handle(buf, ErrNoError);
        co_return 0;
    }

    template <class Resp>
    coev::awaitable<int> DefaultAuthSendReceiver(const std::string &authBytes, Resp &authenticateResponse)
    {
        auto authenticateRequest = CreateSaslAuthenticateRequest(authBytes);
        ResponsePromise<Resp> prom;
        auto err = co_await Send(authenticateRequest, prom);
        if (err)
        {
            co_return err;
        }
        auto packets = co_await prom.m_packets.get();
        if (packets.empty())
        {
            auto err = co_await prom.m_errors.get();
            co_return err;
        }
        SaslAuthenticateResponse tempResponse;
        err = versionedDecode(packets, tempResponse, authenticateRequest.version());
        if (err)
        {
            co_return err;
        }
        authenticateResponse = tempResponse;
        co_return 0;
    }
};
std::shared_ptr<tls::Config> ValidServerNameTLS(const std::string &addr, std::shared_ptr<tls::Config> cfg);