#pragma once

#include <memory>
#include <string>
#include <map>
#include <atomic>
#include <mutex>
#include <chrono>
#include <coev/coev.h>

#include "config.h"
#include "connect.h"
#include "client.h"
#include "metrics.h"
#include "request.h"
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "metadata_request.h"
#include "metadata_response.h"
#include "consumer_metadata_request.h"
#include "consumer_metadata_response.h"
#include "find_coordinator_request.h"
#include "find_coordinator_response.h"
#include "offset_request.h"
#include "offset_response.h"
#include "produce_request.h"
#include "produce_response.h"
#include "fetch_request.h"
#include "fetch_response.h"
#include "offset_commit_request.h"
#include "offset_commit_response.h"
#include "offset_fetch_request.h"
#include "offset_fetch_response.h"
#include "join_group_request.h"
#include "join_group_response.h"
#include "sync_group_request.h"
#include "sync_group_response.h"
#include "leave_group_request.h"
#include "leave_group_response.h"
#include "heartbeat_request.h"
#include "heartbeat_response.h"
#include "list_groups_request.h"
#include "list_groups_response.h"
#include "describe_groups_request.h"
#include "describe_groups_response.h"
#include "api_versions_request.h"
#include "api_versions_response.h"
#include "create_topics_request.h"
#include "create_topics_response.h"
#include "delete_topics_request.h"
#include "delete_topics_response.h"
#include "create_partitions_request.h"
#include "create_partitions_response.h"
#include "alter_partition_reassignments_request.h"
#include "alter_partition_reassignments_response.h"
#include "list_partition_reassignments_request.h"
#include "list_partition_reassignments_response.h"
#include "elect_leaders_request.h"
#include "elect_leaders_response.h"
#include "delete_records_request.h"
#include "delete_records_response.h"
#include "acl_describe_request.h"
#include "acl_describe_response.h"
#include "acl_create_request.h"
#include "acl_create_response.h"
#include "acl_delete_request.h"
#include "acl_delete_response.h"
#include "init_producer_id_request.h"
#include "init_producer_id_response.h"
#include "add_partitions_to_txn_request.h"
#include "add_partitions_to_txn_response.h"
#include "add_offsets_to_txn_request.h"
#include "add_offsets_to_txn_response.h"
#include "end_txn_request.h"
#include "end_txn_response.h"
#include "txn_offset_commit_request.h"
#include "txn_offset_commit_response.h"
#include "describe_configs_request.h"
#include "describe_configs_response.h"
#include "alter_configs_request.h"
#include "alter_configs_response.h"
#include "incremental_alter_configs_request.h"
#include "incremental_alter_configs_response.h"
#include "delete_groups_request.h"
#include "delete_groups_response.h"
#include "delete_offsets_request.h"
#include "delete_offsets_response.h"
#include "describe_log_dirs_request.h"
#include "describe_log_dirs_response.h"
#include "describe_user_scram_credentials_request.h"
#include "describe_user_scram_credentials_response.h"
#include "alter_user_scram_credentials_request.h"
#include "alter_user_scram_credentials_response.h"
#include "describe_client_quotas_request.h"
#include "describe_client_quotas_response.h"
#include "alter_client_quotas_request.h"
#include "alter_client_quotas_response.h"
#include "sasl_handshake_request.h"
#include "sasl_handshake_response.h"
#include "sasl_authenticate_response.h"
#include "sasl_authenticate_request.h"
#include "logger.h"
#include "gssapi_kerberos.h"
#include "access_token.h"
#include "response_promise.h"
#include "undefined.h"
#include "api_versions.h"
#include "tls.h"
#include "connect.h"

struct GSSAPIKerberosAuth;

struct Broker : VEncoder, VDecoder, std::enable_shared_from_this<Broker>
{

    Broker() = default;
    Broker(const std::string &addr);
    Broker(int id, const std::string &addr);

    coev::awaitable<int> Open(std::shared_ptr<Config> conf);
    int ResponseSize();
    bool Connected();
    int TLSConnectionState();
    int32_t ID();
    std::string Addr();
    std::string Rack();

    coev::awaitable<int> Close();
    coev::awaitable<int> GetMetadata(std::shared_ptr<MetadataRequest> request, std::shared_ptr<MetadataResponse> &response);
    coev::awaitable<int> GetConsumerMetadata(std::shared_ptr<ConsumerMetadataRequest> request, std::shared_ptr<ConsumerMetadataResponse> &response);
    coev::awaitable<int> FindCoordinator(std::shared_ptr<FindCoordinatorRequest> request, std::shared_ptr<FindCoordinatorResponse> &response);
    coev::awaitable<int> GetAvailableOffsets(std::shared_ptr<OffsetRequest> request, std::shared_ptr<OffsetResponse> &response);
    coev::awaitable<int> Produce(std::shared_ptr<ProduceRequest> request, std::shared_ptr<ProduceResponse> &response);
    coev::awaitable<int> Fetch(std::shared_ptr<FetchRequest> request, std::shared_ptr<FetchResponse> &response);
    coev::awaitable<int> CommitOffset(std::shared_ptr<OffsetCommitRequest> request, std::shared_ptr<OffsetCommitResponse> &response);
    coev::awaitable<int> FetchOffset(std::shared_ptr<OffsetFetchRequest> request, std::shared_ptr<OffsetFetchResponse> &response);
    coev::awaitable<int> JoinGroup(std::shared_ptr<JoinGroupRequest> request, std::shared_ptr<JoinGroupResponse> &response);
    coev::awaitable<int> SyncGroup(std::shared_ptr<SyncGroupRequest> request, std::shared_ptr<SyncGroupResponse> &response);
    coev::awaitable<int> LeaveGroup(std::shared_ptr<LeaveGroupRequest> request, std::shared_ptr<LeaveGroupResponse> &response);
    coev::awaitable<int> Heartbeat(std::shared_ptr<HeartbeatRequest> request, std::shared_ptr<HeartbeatResponse> &response);
    coev::awaitable<int> ListGroups(std::shared_ptr<ListGroupsRequest> request, std::shared_ptr<ListGroupsResponse> &response);
    coev::awaitable<int> DescribeGroups(std::shared_ptr<DescribeGroupsRequest> request, std::shared_ptr<DescribeGroupsResponse> &response);
    coev::awaitable<int> ApiVersions(std::shared_ptr<ApiVersionsRequest> request, std::shared_ptr<ApiVersionsResponse> &response);
    coev::awaitable<int> CreateTopics(std::shared_ptr<CreateTopicsRequest> request, std::shared_ptr<CreateTopicsResponse> &response);
    coev::awaitable<int> DeleteTopics(std::shared_ptr<DeleteTopicsRequest> request, std::shared_ptr<DeleteTopicsResponse> &response);
    coev::awaitable<int> CreatePartitions(std::shared_ptr<CreatePartitionsRequest> request, std::shared_ptr<CreatePartitionsResponse> &response);
    coev::awaitable<int> AlterPartitionReassignments(std::shared_ptr<AlterPartitionReassignmentsRequest> request, std::shared_ptr<AlterPartitionReassignmentsResponse> &response);
    coev::awaitable<int> ListPartitionReassignments(std::shared_ptr<ListPartitionReassignmentsRequest> request, std::shared_ptr<ListPartitionReassignmentsResponse> &response);
    coev::awaitable<int> ElectLeaders(std::shared_ptr<ElectLeadersRequest> request, std::shared_ptr<ElectLeadersResponse> &response);
    coev::awaitable<int> DeleteRecords(std::shared_ptr<DeleteRecordsRequest> request, std::shared_ptr<DeleteRecordsResponse> &response);
    coev::awaitable<int> DescribeAcls(std::shared_ptr<DescribeAclsRequest> request, std::shared_ptr<DescribeAclsResponse> &response);
    coev::awaitable<int> CreateAcls(std::shared_ptr<CreateAclsRequest> request, std::shared_ptr<CreateAclsResponse> &response);
    coev::awaitable<int> DeleteAcls(std::shared_ptr<DeleteAclsRequest> request, std::shared_ptr<DeleteAclsResponse> &response);
    coev::awaitable<int> InitProducerID(std::shared_ptr<InitProducerIDRequest> request, std::shared_ptr<InitProducerIDResponse> &response);
    coev::awaitable<int> AddPartitionsToTxn(std::shared_ptr<AddPartitionsToTxnRequest> request, std::shared_ptr<AddPartitionsToTxnResponse> &response);
    coev::awaitable<int> AddOffsetsToTxn(std::shared_ptr<AddOffsetsToTxnRequest> request, std::shared_ptr<AddOffsetsToTxnResponse> &response);
    coev::awaitable<int> EndTxn(std::shared_ptr<EndTxnRequest> request, std::shared_ptr<EndTxnResponse> &response);
    coev::awaitable<int> TxnOffsetCommit(std::shared_ptr<TxnOffsetCommitRequest> request, std::shared_ptr<TxnOffsetCommitResponse> &response);
    coev::awaitable<int> DescribeConfigs(std::shared_ptr<DescribeConfigsRequest> request, std::shared_ptr<DescribeConfigsResponse> &response);
    coev::awaitable<int> AlterConfigs(std::shared_ptr<AlterConfigsRequest> request, std::shared_ptr<AlterConfigsResponse> &response);
    coev::awaitable<int> IncrementalAlterConfigs(std::shared_ptr<IncrementalAlterConfigsRequest> request, std::shared_ptr<IncrementalAlterConfigsResponse> &response);
    coev::awaitable<int> DeleteGroups(std::shared_ptr<DeleteGroupsRequest> request, std::shared_ptr<DeleteGroupsResponse> &response);
    coev::awaitable<int> DeleteOffsets(std::shared_ptr<DeleteOffsetsRequest> request, std::shared_ptr<DeleteOffsetsResponse> &response);
    coev::awaitable<int> DescribeLogDirs(std::shared_ptr<DescribeLogDirsRequest> request, std::shared_ptr<DescribeLogDirsResponse> &response);
    coev::awaitable<int> DescribeUserScramCredentials(std::shared_ptr<DescribeUserScramCredentialsRequest> req, std::shared_ptr<DescribeUserScramCredentialsResponse> &res);
    coev::awaitable<int> AlterUserScramCredentials(std::shared_ptr<AlterUserScramCredentialsRequest> req, std::shared_ptr<AlterUserScramCredentialsResponse> &res);
    coev::awaitable<int> DescribeClientQuotas(std::shared_ptr<DescribeClientQuotasRequest> request, std::shared_ptr<DescribeClientQuotasResponse> &response);
    coev::awaitable<int> AlterClientQuotas(std::shared_ptr<AlterClientQuotasRequest> request, std::shared_ptr<AlterClientQuotasResponse> &response);
    coev::awaitable<int> AsyncProduce(std::shared_ptr<ProduceRequest> request, std::function<void(std::shared_ptr<ProduceResponse>, KError)> callback);

    std::shared_ptr<Config> m_Conf;
    std::string m_Rack;
    int32_t m_ID;
    std::string m_Addr;
    int32_t m_CorrelationID;
    Connect m_Conn;
    std::mutex m_Lock;
    std::atomic<bool> m_Opened;

    std::shared_ptr<metrics::Registry> metricRegistry;
    std::shared_ptr<metrics::Meter> incomingByteRate;
    std::shared_ptr<metrics::Meter> requestRate;
    std::shared_ptr<metrics::Meter> fetchRate;
    std::shared_ptr<metrics::Histogram> requestSize;
    std::shared_ptr<metrics::Histogram> requestLatency;
    std::shared_ptr<metrics::Meter> outgoingByteRate;
    std::shared_ptr<metrics::Meter> responseRate;
    std::shared_ptr<metrics::Histogram> responseSize;
    std::shared_ptr<metrics::Counter> requestsInFlight;
    std::shared_ptr<metrics::Meter> brokerIncomingByteRate;
    std::shared_ptr<metrics::Meter> brokerRequestRate;
    std::shared_ptr<metrics::Meter> brokerFetchRate;
    std::shared_ptr<metrics::Histogram> brokerRequestSize;
    std::shared_ptr<metrics::Histogram> brokerRequestLatency;
    std::shared_ptr<metrics::Meter> brokerOutgoingByteRate;
    std::shared_ptr<metrics::Meter> brokerResponseRate;
    std::shared_ptr<metrics::Histogram> brokerResponseSize;
    std::shared_ptr<metrics::Counter> brokerRequestsInFlight;
    std::shared_ptr<metrics::Histogram> brokerThrottleTime;
    std::map<int16_t, std::shared_ptr<metrics::Meter>> protocolRequestsRate;
    std::map<int16_t, std::shared_ptr<metrics::Meter>> brokerProtocolRequestsRate;
    ApiVersionMap brokerAPIVersions;
    std::shared_ptr<GSSAPIKerberosAuth> kerberosAuthenticator;
    int64_t clientSessionReauthenticationTime;

    std::mutex throttleTimerLock;
    coev::co_task m_task;
    coev::co_channel<std::shared_ptr<ResponsePromise>> responses;
    coev::co_channel<bool> done;

    std::shared_ptr<ResponsePromise> makeResponsePromise(std::shared_ptr<protocolBody> res);
    coev::awaitable<int> ReadFull(std::string &buf, size_t &n);
    coev::awaitable<int> Write(const std::string &buf, size_t &n);
    coev::awaitable<int> Send(std::shared_ptr<protocolBody> req, std::shared_ptr<protocolBody> res, std::shared_ptr<ResponsePromise> &promise);
    coev::awaitable<int> SendWithPromise(std::shared_ptr<protocolBody> rb, std::shared_ptr<ResponsePromise> promise);
    coev::awaitable<int> SendInternal(std::shared_ptr<protocolBody> rb, std::shared_ptr<ResponsePromise> promise);
    coev::awaitable<int> SendAndReceive(std::shared_ptr<protocolBody> req, std::shared_ptr<protocolBody> res);
    coev::awaitable<int> SesponseReceiver();
    coev::awaitable<int> SendAndReceiveApiVersions(int16_t v, std::shared_ptr<ApiVersionsResponse> &response);
    coev::awaitable<int> AuthenticateViaSASLv0();
    coev::awaitable<int> AuthenticateViaSASLv1();
    coev::awaitable<int> SendAndReceiveKerberos();
    coev::awaitable<int> SendAndReceiveSASLHandshake(const std::string &saslType, int16_t version);
    coev::awaitable<int> SendAndReceiveSASLPlainAuthV0();
    coev::awaitable<int> SendAndReceiveSASLPlainAuthV1(AuthSendReceiver authSendReceiver);
    coev::awaitable<int> SendAndReceiveSASLOAuth(std::shared_ptr<AccessTokenProvider> provider, AuthSendReceiver authSendReceiver);
    coev::awaitable<int> SendAndReceiveSASLSCRAMv0();
    coev::awaitable<int> SendAndReceiveSASLSCRAMv1(std::shared_ptr<SCRAMClient> scramClient, AuthSendReceiver authSendReceiver);
    coev::awaitable<int> DefaultAuthSendReceiver(const std::string &authBytes, std::shared_ptr<SaslAuthenticateResponse> &authenticateResponse);
    coev::awaitable<int> SendOffsetCommit(std::shared_ptr<OffsetCommitRequest> req, std::shared_ptr<OffsetCommitResponse> &resp, std::shared_ptr<ResponsePromise> &promise);
    std::shared_ptr<SaslAuthenticateRequest> CreateSaslAuthenticateRequest(const std::string &msg);

    int decode(PDecoder &pd, int16_t version);
    int encode(PEncoder &pe, int16_t version);
    void safeAsyncClose();

    int BuildClientFirstMessage(std::shared_ptr<AccessToken> token, std::string &);
    std::string mapToString(const std::map<std::string, std::string> &extensions, const std::string &keyValSep, const std::string &elemSep);
    int64_t CurrentUnixMilli();
    void computeSaslSessionLifetime(std::shared_ptr<SaslAuthenticateResponse> res);
    void UpdateIncomingCommunicationMetrics(int bytes, std::chrono::milliseconds requestLatency);
    void UpdateRequestLatencyAndInFlightMetrics(std::chrono::milliseconds requestLatency);
    void AddRequestInFlightMetrics(int64_t i);
    void UpdateOutgoingCommunicationMetrics(int bytes);
    void UpdateProtocolMetrics(std::shared_ptr<protocolBody> rb);
    void UandleThrottledResponse(std::shared_ptr<protocolBody> resp);
    void SetThrottle(std::chrono::milliseconds throttleTime);
    void WaitIfThrottled();
    void UpdateThrottleMetric(std::chrono::milliseconds throttleTime);
    void RegisterMetrics();

    std::shared_ptr<metrics::Meter> RegisterMeter(const std::string &name);
    std::shared_ptr<metrics::Histogram> RegisterHistogram(const std::string &name);
    std::shared_ptr<metrics::Counter> RegisterCounter(const std::string &name);

    template <class Req, class Resp>
    coev::awaitable<int> SendAndReceive(Req &req, Resp &res)
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        std::shared_ptr<ResponsePromise> promise;
        int32_t err = co_await Send(req, res, promise);
        if (err)
        {
            co_return err;
        }
        if (!promise)
        {
            co_return 0;
        }
        err = co_await HandleResponsePromise(req, res, promise, metricRegistry);
        if (err)
        {
            co_return err;
        }
        if (res)
        {
            UandleThrottledResponse(res);
        }
        co_return 0;
    }
};

std::shared_ptr<tls::Config> ValidServerNameTLS(const std::string &addr, std::shared_ptr<tls::Config> cfg);
coev::awaitable<int> HandleResponsePromise(std::shared_ptr<protocolBody> req, std::shared_ptr<protocolBody> res, std::shared_ptr<ResponsePromise> promise, std::shared_ptr<metrics::Registry> metricRegistry);
