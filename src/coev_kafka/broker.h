#pragma once

#include <atomic>
#include <chrono>
#include <coev/coev.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <functional>
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
#include "response_header.h"
#include "txn_offset_commit_request.h"
#include "txn_offset_commit_response.h"
#include "undefined.h"

int8_t getHeaderLength(int16_t header_version);

struct GSSAPIKerberosAuth;

struct Broker : versioned_encoder, versioned_decoder, std::enable_shared_from_this<Broker>
{

    Broker() = default;
    Broker(const std::string &addr);
    Broker(int id, const std::string &addr);
    ~Broker();

    bool Connected();
    int TLSConnectionState();

    coev::awaitable<int> Open(std::shared_ptr<Config> conf);
    coev::awaitable<int> Close();
    coev::awaitable<int> GetMetadata(std::shared_ptr<MetadataRequest> request, ResponsePromise<MetadataResponse> &response);
    coev::awaitable<int> GetConsumerMetadata(std::shared_ptr<ConsumerMetadataRequest> request, ResponsePromise<ConsumerMetadataResponse> &response);
    coev::awaitable<int> FindCoordinator(std::shared_ptr<FindCoordinatorRequest> request, ResponsePromise<FindCoordinatorResponse> &response);
    coev::awaitable<int> GetAvailableOffsets(std::shared_ptr<OffsetRequest> request, ResponsePromise<OffsetResponse> &response);
    coev::awaitable<int> Produce(std::shared_ptr<ProduceRequest> request, ResponsePromise<ProduceResponse> &response);
    coev::awaitable<int> Fetch(std::shared_ptr<FetchRequest> request, ResponsePromise<FetchResponse> &response);
    coev::awaitable<int> CommitOffset(std::shared_ptr<OffsetCommitRequest> request, ResponsePromise<OffsetCommitResponse> &response);
    coev::awaitable<int> FetchOffset(std::shared_ptr<OffsetFetchRequest> request, ResponsePromise<OffsetFetchResponse> &response);
    coev::awaitable<int> JoinGroup(std::shared_ptr<JoinGroupRequest> request, ResponsePromise<JoinGroupResponse> &response);
    coev::awaitable<int> SyncGroup(std::shared_ptr<SyncGroupRequest> request, ResponsePromise<SyncGroupResponse> &response);
    coev::awaitable<int> LeaveGroup(std::shared_ptr<LeaveGroupRequest> request, ResponsePromise<LeaveGroupResponse> &response);
    coev::awaitable<int> Heartbeat(std::shared_ptr<HeartbeatRequest> request, ResponsePromise<HeartbeatResponse> &response);
    coev::awaitable<int> ListGroups(std::shared_ptr<ListGroupsRequest> request, ResponsePromise<ListGroupsResponse> &response);
    coev::awaitable<int> DescribeGroups(std::shared_ptr<DescribeGroupsRequest> request, ResponsePromise<DescribeGroupsResponse> &response);
    coev::awaitable<int> ApiVersions(std::shared_ptr<ApiVersionsRequest> request, ResponsePromise<ApiVersionsResponse> &response);
    coev::awaitable<int> CreateTopics(std::shared_ptr<CreateTopicsRequest> request, ResponsePromise<CreateTopicsResponse> &response);
    coev::awaitable<int> DeleteTopics(std::shared_ptr<DeleteTopicsRequest> request, ResponsePromise<DeleteTopicsResponse> &response);
    coev::awaitable<int> CreatePartitions(std::shared_ptr<CreatePartitionsRequest> request, ResponsePromise<CreatePartitionsResponse> &response);
    coev::awaitable<int> AlterPartitionReassignments(std::shared_ptr<AlterPartitionReassignmentsRequest> request, ResponsePromise<AlterPartitionReassignmentsResponse> &response);
    coev::awaitable<int> ListPartitionReassignments(std::shared_ptr<ListPartitionReassignmentsRequest> request, ResponsePromise<ListPartitionReassignmentsResponse> &response);
    coev::awaitable<int> ElectLeaders(std::shared_ptr<ElectLeadersRequest> request, ResponsePromise<ElectLeadersResponse> &response);
    coev::awaitable<int> DeleteRecords(std::shared_ptr<DeleteRecordsRequest> request, ResponsePromise<DeleteRecordsResponse> &response);
    coev::awaitable<int> DescribeAcls(std::shared_ptr<DescribeAclsRequest> request, ResponsePromise<DescribeAclsResponse> &response);
    coev::awaitable<int> CreateAcls(std::shared_ptr<CreateAclsRequest> request, ResponsePromise<CreateAclsResponse> &response);
    coev::awaitable<int> DeleteAcls(std::shared_ptr<DeleteAclsRequest> request, ResponsePromise<DeleteAclsResponse> &response);
    coev::awaitable<int> InitProducerID(std::shared_ptr<InitProducerIDRequest> request, ResponsePromise<InitProducerIDResponse> &response);
    coev::awaitable<int> AddPartitionsToTxn(std::shared_ptr<AddPartitionsToTxnRequest> request, ResponsePromise<AddPartitionsToTxnResponse> &response);
    coev::awaitable<int> AddOffsetsToTxn(std::shared_ptr<AddOffsetsToTxnRequest> request, ResponsePromise<AddOffsetsToTxnResponse> &response);
    coev::awaitable<int> EndTxn(std::shared_ptr<EndTxnRequest> request, ResponsePromise<EndTxnResponse> &response);
    coev::awaitable<int> TxnOffsetCommit(std::shared_ptr<TxnOffsetCommitRequest> request, ResponsePromise<TxnOffsetCommitResponse> &response);
    coev::awaitable<int> DescribeConfigs(std::shared_ptr<DescribeConfigsRequest> request, ResponsePromise<DescribeConfigsResponse> &response);
    coev::awaitable<int> AlterConfigs(std::shared_ptr<AlterConfigsRequest> request, ResponsePromise<AlterConfigsResponse> &response);
    coev::awaitable<int> IncrementalAlterConfigs(std::shared_ptr<IncrementalAlterConfigsRequest> request, ResponsePromise<IncrementalAlterConfigsResponse> &response);
    coev::awaitable<int> DeleteGroups(std::shared_ptr<DeleteGroupsRequest> request, ResponsePromise<DeleteGroupsResponse> &response);
    coev::awaitable<int> DeleteOffsets(std::shared_ptr<DeleteOffsetsRequest> request, ResponsePromise<DeleteOffsetsResponse> &response);
    coev::awaitable<int> DescribeLogDirs(std::shared_ptr<DescribeLogDirsRequest> request, ResponsePromise<DescribeLogDirsResponse> &response);
    coev::awaitable<int> DescribeUserScramCredentials(std::shared_ptr<DescribeUserScramCredentialsRequest> req, ResponsePromise<DescribeUserScramCredentialsResponse> &res);
    coev::awaitable<int> AlterUserScramCredentials(std::shared_ptr<AlterUserScramCredentialsRequest> req, ResponsePromise<AlterUserScramCredentialsResponse> &res);
    coev::awaitable<int> DescribeClientQuotas(std::shared_ptr<DescribeClientQuotasRequest> request, ResponsePromise<DescribeClientQuotasResponse> &response);
    coev::awaitable<int> AlterClientQuotas(std::shared_ptr<AlterClientQuotasRequest> request, ResponsePromise<AlterClientQuotasResponse> &response);

    int decode(packetDecoder &pd, int16_t version);
    int encode(packetEncoder &pe, int16_t version);

    std::string Addr();
    std::string Rack();
    int32_t ID();

    int32_t m_id;
    std::string m_rack;
    std::string m_addr;

    std::unique_ptr<Connect> m_conn;
    std::shared_ptr<Config> m_conf;
    std::shared_ptr<GSSAPIKerberosAuth> m_kerberos_authenticator;

    int32_t m_correlation_id;
    int64_t m_session_reauthentication_time;
    ApiVersionMap m_broker_api_versions;

    coev::async m_opened;
    coev::co_task m_task;
    coev::co_channel<bool> m_done;

    coev::awaitable<int> ReadFull(std::string &buf, size_t n);
    coev::awaitable<int> Write(const std::string &buf);

    coev::awaitable<int> AuthenticateViaSASLv0();
    coev::awaitable<int> AuthenticateViaSASLv1();
    coev::awaitable<int> SendAndReceiveKerberos();
    coev::awaitable<int> SendAndReceiveSASLHandshake(const std::string &saslType, int16_t version);
    coev::awaitable<int> SendAndReceiveSASLPlainAuthV0();
    coev::awaitable<int> SendAndReceiveSASLPlainAuthV1(AuthSendReceiver authSendReceiver);
    coev::awaitable<int> SendAndReceiveSASLOAuth(std::shared_ptr<AccessTokenProvider> provider, AuthSendReceiver authSendReceiver);
    coev::awaitable<int> SendAndReceiveSASLSCRAMv0();
    coev::awaitable<int> SendAndReceiveSASLSCRAMv1(std::shared_ptr<SCRAMClient> scramClient, AuthSendReceiver authSendReceiver);
    coev::awaitable<int> SendAndReceiveApiVersions(int16_t v, ResponsePromise<ApiVersionsResponse> &promise);
    coev::awaitable<int> _Open();

    int CreateSaslAuthenticateRequest(const std::string &msg, SaslAuthenticateRequest &);
    void SafeAsyncClose();
    int BuildClientFirstMessage(std::shared_ptr<AccessToken> token, std::string &);
    std::string MapToString(const std::map<std::string, std::string> &extensions, const std::string &keyValSep, const std::string &elemSep);
    int64_t CurrentUnixMilli();
    void ComputeSaslSessionLifetime(std::shared_ptr<SaslAuthenticateResponse> res);

    template <class Req, class Res>
    coev::awaitable<int> SendAndReceive(std::shared_ptr<Req> request, ResponsePromise<Res> &promise)
    {
        int err = 0;
        if (m_session_reauthentication_time > 0 && CurrentUnixMilli() > m_session_reauthentication_time)
        {
            err = co_await AuthenticateViaSASLv1();
            if (err != ErrNoError)
            {
                co_return err;
            }
        }
        err = co_await SendInternal(request, promise);
        if (err != ErrNoError)
        {
            co_return err;
        }
        err = co_await ResponseReceiver(request, promise, err);
        if (err != ErrNoError)
        {
            co_return err;
        }

        co_return err;
    }

    template <class Req, class Res>
    coev::awaitable<int> SendInternal(std::shared_ptr<Req> request, ResponsePromise<Res> &promise)
    {
        RestrictApiVersion(*request, m_broker_api_versions);
        Request _request;
        _request.m_correlation_id = m_correlation_id;
        _request.m_client_id = m_conf->ClientID;
        _request.m_body = request.get();
        std::string buf;
        ::encode(_request, buf);

        auto requestTime = std::chrono::system_clock::now();
        auto err = co_await Write(buf);
        if (err)
        {
            LOG_CORE("Write %d %s", errno, strerror(errno));
            co_return err;
        }

        promise.m_request_time = requestTime;
        promise.m_correlation_id = _request.m_correlation_id;
        m_correlation_id++;
        co_return ErrNoError;
    }

    template <class Req, class Res>
    coev::awaitable<int> ResponseReceiver(std::shared_ptr<Req> request, ResponsePromise<Res> &promise, int &err)
    {
        std::string header;
        auto bytesReadHeader = getHeaderLength(promise.m_response.header_version());
        err = co_await ReadFull(header, bytesReadHeader);
        auto m_request_latency = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - promise.m_request_time);
        if (err)
        {
            co_return err;
        }
        ResponseHeader decoded_header;
        err = versionedDecode(header, decoded_header, promise.m_response.header_version());
        if (err)
        {
            co_return err;
        }
        if (decoded_header.m_correlation_id != promise.m_correlation_id)
        {

            co_return ErrCorrelationID;
        }

        size_t bytesReadBody = decoded_header.m_length - bytesReadHeader + 4;
        err = co_await ReadFull(promise.m_packets, bytesReadBody);
        if (err)
        {
            co_return err;
        }
        if (promise.m_packets.size() != bytesReadBody)
        {
            co_return err;
        }
        err = promise.decode(request->version());
        if (err)
        {
            co_return err;
        }
        co_return ErrNoError;
    }

    template <class Res>
    coev::awaitable<int> DefaultAuthSendReceiver(const std::string &authBytes, ResponsePromise<Res> &promise)
    {
        SaslAuthenticateRequest authenticateRequest;
        auto err = CreateSaslAuthenticateRequest(authBytes, authenticateRequest);
        if (err)
        {
            co_return err;
        }
        err = co_await SendAndReceive(std::make_shared<SaslAuthenticateRequest>(authenticateRequest), promise);
        if (err)
        {
            co_return err;
        }
        err = promise.decode(authenticateRequest.version());
        if (err)
        {
            co_return err;
        }

        co_return 0;
    }
    template <class Func, class... Args>
    coev::awaitable<int> AsyncProduce(std::shared_ptr<ProduceRequest> request, const Func &_f, Args &&...args)
    {
        bool needAcks = request->m_acks != NoResponse;
        if (needAcks)
        {
            m_task << [](auto _this, auto request, const auto &_f, auto &&...args) -> coev::awaitable<void>
            {
                ResponsePromise<ProduceResponse> response;
                auto err = co_await _this->SendAndReceive(request, response);
                _f(response, err, std::forward<decltype(args)>(args)...);
            }(shared_from_this(), request, _f, std::forward<Args>(args)...);
        }
        else
        {
            ResponsePromise<ProduceResponse> response;
            auto err = co_await SendAndReceive(request, response);
            _f(response, err, std::forward<decltype(args)>(args)...);
        }
        co_return 0;
    }
};
std::shared_ptr<tls::Config> ValidServerNameTLS(const std::string &addr, std::shared_ptr<tls::Config> cfg);