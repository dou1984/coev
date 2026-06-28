/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <atomic>
#include <chrono>
#include <coev/coev.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <functional>
#include <utils/convert.h>
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

namespace coev::kafka
{

    struct GSSAPIKerberosAuth;

    struct Broker : VEncoder, VDecoder, std::enable_shared_from_this<Broker>
    {
        Broker();
        Broker(const std::string &addr);
        Broker(int id, const std::string &addr);
        ~Broker();

        bool Connected();
        int Close();
        int TLSConnectionState();
        std::string Addr();
        std::string Rack();
        int32_t ID();

        awaitable<int> Open(std::shared_ptr<Config> conf);
        awaitable<int> GetMetadata(std::shared_ptr<MetadataRequest> request, ResponsePromise<MetadataResponse> &response);
        awaitable<int> GetConsumerMetadata(std::shared_ptr<ConsumerMetadataRequest> request, ResponsePromise<ConsumerMetadataResponse> &response);
        awaitable<int> FindCoordinator(std::shared_ptr<FindCoordinatorRequest> request, ResponsePromise<FindCoordinatorResponse> &response);
        awaitable<int> GetAvailableOffsets(std::shared_ptr<OffsetRequest> request, ResponsePromise<OffsetResponse> &response);
        awaitable<int> Produce(std::shared_ptr<ProduceRequest> request, ResponsePromise<ProduceResponse> &response);
        awaitable<int> Fetch(std::shared_ptr<FetchRequest> request, ResponsePromise<FetchResponse> &response);
        awaitable<int> CommitOffset(std::shared_ptr<OffsetCommitRequest> request, ResponsePromise<OffsetCommitResponse> &response);
        awaitable<int> FetchOffset(std::shared_ptr<OffsetFetchRequest> request, ResponsePromise<OffsetFetchResponse> &response);
        awaitable<int> JoinGroup(std::shared_ptr<JoinGroupRequest> request, ResponsePromise<JoinGroupResponse> &response);
        awaitable<int> SyncGroup(std::shared_ptr<SyncGroupRequest> request, ResponsePromise<SyncGroupResponse> &response);
        awaitable<int> LeaveGroup(std::shared_ptr<LeaveGroupRequest> request, ResponsePromise<LeaveGroupResponse> &response);
        awaitable<int> Heartbeat(std::shared_ptr<HeartbeatRequest> request, ResponsePromise<HeartbeatResponse> &response);
        awaitable<int> ListGroups(std::shared_ptr<ListGroupsRequest> request, ResponsePromise<ListGroupsResponse> &response);
        awaitable<int> DescribeGroups(std::shared_ptr<DescribeGroupsRequest> request, ResponsePromise<DescribeGroupsResponse> &response);
        awaitable<int> ApiVersions(std::shared_ptr<ApiVersionsRequest> request, ResponsePromise<ApiVersionsResponse> &response);
        awaitable<int> CreateTopics(std::shared_ptr<CreateTopicsRequest> request, ResponsePromise<CreateTopicsResponse> &response);
        awaitable<int> DeleteTopics(std::shared_ptr<DeleteTopicsRequest> request, ResponsePromise<DeleteTopicsResponse> &response);
        awaitable<int> CreatePartitions(std::shared_ptr<CreatePartitionsRequest> request, ResponsePromise<CreatePartitionsResponse> &response);
        awaitable<int> AlterPartitionReassignments(std::shared_ptr<AlterPartitionReassignmentsRequest> request, ResponsePromise<AlterPartitionReassignmentsResponse> &response);
        awaitable<int> ListPartitionReassignments(std::shared_ptr<ListPartitionReassignmentsRequest> request, ResponsePromise<ListPartitionReassignmentsResponse> &response);
        awaitable<int> ElectLeaders(std::shared_ptr<ElectLeadersRequest> request, ResponsePromise<ElectLeadersResponse> &response);
        awaitable<int> DeleteRecords(std::shared_ptr<DeleteRecordsRequest> request, ResponsePromise<DeleteRecordsResponse> &response);
        awaitable<int> DescribeAcls(std::shared_ptr<DescribeAclsRequest> request, ResponsePromise<DescribeAclsResponse> &response);
        awaitable<int> CreateAcls(std::shared_ptr<CreateAclsRequest> request, ResponsePromise<CreateAclsResponse> &response);
        awaitable<int> DeleteAcls(std::shared_ptr<DeleteAclsRequest> request, ResponsePromise<DeleteAclsResponse> &response);
        awaitable<int> InitProducerID(std::shared_ptr<InitProducerIDRequest> request, ResponsePromise<InitProducerIDResponse> &response);
        awaitable<int> AddPartitionsToTxn(std::shared_ptr<AddPartitionsToTxnRequest> request, ResponsePromise<AddPartitionsToTxnResponse> &response);
        awaitable<int> AddOffsetsToTxn(std::shared_ptr<AddOffsetsToTxnRequest> request, ResponsePromise<AddOffsetsToTxnResponse> &response);
        awaitable<int> EndTxn(std::shared_ptr<EndTxnRequest> request, ResponsePromise<EndTxnResponse> &response);
        awaitable<int> TxnOffsetCommit(std::shared_ptr<TxnOffsetCommitRequest> request, ResponsePromise<TxnOffsetCommitResponse> &response);
        awaitable<int> DescribeConfigs(std::shared_ptr<DescribeConfigsRequest> request, ResponsePromise<DescribeConfigsResponse> &response);
        awaitable<int> AlterConfigs(std::shared_ptr<AlterConfigsRequest> request, ResponsePromise<AlterConfigsResponse> &response);
        awaitable<int> IncrementalAlterConfigs(std::shared_ptr<IncrementalAlterConfigsRequest> request, ResponsePromise<IncrementalAlterConfigsResponse> &response);
        awaitable<int> DeleteGroups(std::shared_ptr<DeleteGroupsRequest> request, ResponsePromise<DeleteGroupsResponse> &response);
        awaitable<int> DeleteOffsets(std::shared_ptr<DeleteOffsetsRequest> request, ResponsePromise<DeleteOffsetsResponse> &response);
        awaitable<int> DescribeLogDirs(std::shared_ptr<DescribeLogDirsRequest> request, ResponsePromise<DescribeLogDirsResponse> &response);
        awaitable<int> DescribeUserScramCredentials(std::shared_ptr<DescribeUserScramCredentialsRequest> req, ResponsePromise<DescribeUserScramCredentialsResponse> &res);
        awaitable<int> AlterUserScramCredentials(std::shared_ptr<AlterUserScramCredentialsRequest> req, ResponsePromise<AlterUserScramCredentialsResponse> &res);
        awaitable<int> DescribeClientQuotas(std::shared_ptr<DescribeClientQuotasRequest> request, ResponsePromise<DescribeClientQuotasResponse> &response);
        awaitable<int> AlterClientQuotas(std::shared_ptr<AlterClientQuotasRequest> request, ResponsePromise<AlterClientQuotasResponse> &response);

        int decode(PacketDecoder &pd, int16_t version);
        int encode(PacketEncoder &pe, int16_t version) const;

        int32_t m_id;
        std::string m_rack;
        std::string m_addr;

        std::unique_ptr<Connect> m_conn = nullptr;
        std::shared_ptr<Config> m_conf;
        std::shared_ptr<GSSAPIKerberosAuth> m_kerberos_authenticator;

        int32_t m_correlation_id;
        int64_t m_session_reauthentication_time;
        ApiVersionMap m_broker_api_versions;

        coev::co_async m_opened;
        coev::co_task m_task;
        coev::co_channel<bool> m_done;

        awaitable<int> Ready();
        awaitable<int> ReadFull(std::string &buf, size_t n);
        awaitable<int> Write(const std::string &buf);
        awaitable<int> ReceiveResponse(int32_t expected_correlation_id, int16_t header_version, std::string &body);

        awaitable<int> AuthenticateViaSASLv0();
        awaitable<int> AuthenticateViaSASLv1();
        awaitable<int> SendAndReceiveKerberos();
        awaitable<int> SendAndReceiveSASLHandshake(const std::string &saslType, int16_t version);
        awaitable<int> SendAndReceiveSASLPlainAuthV0();
        awaitable<int> SendAndReceiveSASLPlainAuthV1(AuthSendReceiver authSendReceiver);
        awaitable<int> SendAndReceiveSASLOAuth(std::shared_ptr<AccessTokenProvider> provider, AuthSendReceiver authSendReceiver);
        awaitable<int> SendAndReceiveSASLSCRAMv0();
        awaitable<int> SendAndReceiveSASLSCRAMv1(std::shared_ptr<SCRAMClient> scramClient, AuthSendReceiver authSendReceiver);
        awaitable<int> SendAndReceiveApiVersions(int16_t v, ResponsePromise<ApiVersionsResponse> &promise);
        awaitable<int> _Open();

        void SafeAsyncClose();
        int BuildClientFirstMessage(std::shared_ptr<AccessToken> token, std::string &);
        std::string MapToString(const std::map<std::string, std::string> &extensions, const std::string &keyValSep, const std::string &elemSep);
        int64_t CurrentUnixMilli();
        void ComputeSaslSessionLifetime(std::shared_ptr<SaslAuthenticateResponse> res);
        auto RLock() { return m_conn->m_rlock.lock(); }
        auto RUnlock() { return m_conn->m_rlock.unlock(); }
        auto WLock() { return m_conn->m_wlock.lock(); }
        auto WUnlock() { return m_conn->m_wlock.unlock(); }

        template <class Req, class Res>
        awaitable<int> SendAndReceive(std::shared_ptr<Req> request, ResponsePromise<Res> &promise)
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
            err = co_await Ready();
            if (err)
            {
                co_return err;
            }
            err = co_await SendInternal(request, promise);
            if (err != ErrNoError)
            {
                LOG_DBG("SendAndReceive SendInternal failed apiKey:%d err:%d", request->key(), err);
                co_return err;
            }

            err = co_await ResponseReceiver(request, promise);
            if (err != ErrNoError)
            {
                LOG_DBG("SendAndReceive ResponseReceiver failed apiKey:%d err:%d", request->key(), err);
                co_return err;
            }
            co_return ErrNoError;
        }

        template <class Req, class Res>
        awaitable<int> SendInternal(std::shared_ptr<Req> request, ResponsePromise<Res> &promise)
        {
            RestrictApiVersion(request, m_broker_api_versions);
            Request _request;
            _request.m_correlation_id = m_correlation_id;
            _request.m_client_id = m_conf->ClientID;
            _request.m_body = request.get();
            std::string buf;
            coev::kafka::encode(_request, buf);

            co_await WLock();
            finally(WUnlock());
            auto request_time = std::chrono::system_clock::now();
            auto err = co_await Write(buf);
            if (err)
            {
                LOG_CORE("Write %d %s", errno, strerror(errno));
                co_return err;
            }

            promise.m_request_time = request_time;
            promise.m_correlation_id = _request.m_correlation_id;
            m_correlation_id++;
            co_return ErrNoError;
        }

        template <class Req, class Res>
        awaitable<int> ResponseReceiver(std::shared_ptr<Req> request, ResponsePromise<Res> &promise)
        {
            LOG_DBG("ResponseReceiver trying RLock apiKey:%d correlation:%d",
                    request->key(), promise.m_correlation_id);
            co_await RLock();
            LOG_DBG("ResponseReceiver RLock acquired apiKey:%d correlation:%d",
                    request->key(), promise.m_correlation_id);
            finally(RUnlock());

            std::string fixed_header;
            auto err = co_await ReadFull(fixed_header, 8);
            if (err)
            {
                LOG_CORE("Failed to read fixed header %d", err);
                co_return err;
            }
            PacketDecoder hd(fixed_header);
            ResponseHeader decoded_header;
            if (err = hd.getInt32(decoded_header.m_length); err != 0)
            {
                co_return err;
            }
            if (int e = hd.getInt32(decoded_header.m_correlation_id); e != 0)
            {
                co_return e;
            }

            if (decoded_header.m_correlation_id != promise.m_correlation_id)
            {
                co_return ErrCorrelationID;
            }

            size_t remaining_bytes = decoded_header.m_length - 4;
            LOG_DBG("ResponseReceiver apiKey:%d correlation:%d remaining:%zu m_body.size:%zu m_body.cap:%zu",
                    request->key(), promise.m_correlation_id, remaining_bytes,
                    promise.m_body.size(), promise.m_body.capacity());
            promise.m_body.resize(remaining_bytes);
            err = co_await ReadFull(promise.m_body, remaining_bytes);
            if (err)
            {
                LOG_DBG("ResponseReceiver ReadFull body failed apiKey:%d correlation:%d err:%d remaining:%zu",
                        request->key(), promise.m_correlation_id, err, remaining_bytes);
                co_return err;
            }

            promise.m_response->set_version(request->version());
            size_t body_offset = 0;
            if (promise.m_response->header_version() >= 1)
            {
                PacketDecoder tag_decoder(promise.m_body);
                tag_decoder.__push_flexible();
                finally(tag_decoder.__pop_flexible());
                int32_t dummy = 0;
                tag_decoder.getEmptyTaggedFieldArray(dummy);

                body_offset = tag_decoder.m_offset;
            }

            if (body_offset > promise.m_body.size())
            {
                LOG_DBG("ResponseReceiver body_offset overflow body_offset:%zu m_body.size:%zu",
                        body_offset, promise.m_body.size());
                co_return ErrDecodeError;
            }

            promise.m_packets = std::string_view(promise.m_body).substr(body_offset);

            err = promise.decode(request->version());
            if (err)
            {
                LOG_ERR("ResponseReceiver decode failed apiKey:%d correlation:%d err:%d m_packets.size:%zu body_hex:%.*s",
                        request->key(), promise.m_correlation_id, err, promise.m_packets.size(),
                        (int)to_hex(promise.m_body).size(), to_hex(promise.m_body).data());
                co_return err;
            }
            co_return ErrNoError;
        }

        template <class Res>
        awaitable<int> DefaultAuthSendReceiver(const std::string &auth_bytes, ResponsePromise<Res> &promise)
        {
            auto request = std::make_shared<SaslAuthenticateRequest>(m_conf, auth_bytes);
            int err = co_await SendAndReceive(request, promise);
            if (err)
            {
                co_return err;
            }
            err = promise.decode(request->version());
            if (err)
            {
                co_return err;
            }

            co_return 0;
        }
        template <class Func, class... Args>
        awaitable<int> AsyncProduce(std::shared_ptr<ProduceRequest> request, const Func &func, Args &&...args)
        {
            bool need_acks = request->m_acks != NoResponse;
            if (need_acks)
            {
                m_task << [](auto _this, auto request, const auto &func, auto &&...args) -> awaitable<void>
                {
                    ResponsePromise<ProduceResponse> response;
                    auto err = co_await _this->SendAndReceive(request, response);
                    func(response, err, std::forward<decltype(args)>(args)...);
                }(shared_from_this(), request, func, std::forward<Args>(args)...);
            }
            else
            {
                ResponsePromise<ProduceResponse> response;
                auto err = co_await SendAndReceive(request, response);
                func(response, err, std::forward<decltype(args)>(args)...);
            }
            co_return 0;
        }
        awaitable<int> SyncProduce(std::shared_ptr<ProduceRequest> request, ResponsePromise<ProduceResponse> &response)
        {
            return SendAndReceive(request, response);
        }
    };

} // namespace coev::kafka