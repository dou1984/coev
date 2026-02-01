#include <vector>
#include <stdexcept>
#include <tuple>
#include <iostream>
#include "request.h"
#include "broker.h"
#include "api_versions.h"
#include "protocol_body.h"
#include "length_field.h"
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

int Request::encode(packet_encoder &pe) const
{

    LengthField length_field;
    pe.push(length_field);

    pe.putInt16(m_body->key());
    pe.putInt16(m_body->version());
    pe.putInt32(m_correlation_id);

    int err = 0;
    if (m_body->header_version() >= 1)
    {
        err = pe.putString(m_client_id);
        if (err != 0)
        {
            return err;
        }
    }

    if (m_body->header_version() >= 2)
    {
        pe.putUVarint(0);
    }

    bool is_flexible = false;
    auto f = dynamic_cast<const flexible_version *>(m_body);
    if (f != nullptr)
    {
        is_flexible = f->is_flexible_version(m_body->version());
    }

    if (is_flexible)
    {
        pe.pushFlexible();
        defer(pe.popFlexible());

        err = const_cast<protocol_body *>(m_body)->encode(pe);
        if (err != 0)
        {
            return err;
        }
        return pe.pop();
    }
    else
    {
        err = const_cast<protocol_body *>(m_body)->encode(pe);
        if (err != 0)
        {
            return err;
        }
        return pe.pop();
    }
}

int Request::decode(packet_decoder &pd)
{
    int16_t key, version;

    std::string clientID;
    int err;

    err = pd.getInt16(key);
    if (err != 0)
        return err;

    err = pd.getInt16(version);
    if (err != 0)
        return err;

    int32_t correlationID = 0;
    err = pd.getInt32(correlationID);
    if (err != 0)
        return err;
    m_correlation_id = correlationID;

    err = pd.getString(clientID);
    if (err != 0)
        return err;
    m_client_id = clientID;

    auto body = allocateBody(key, version);
    m_body = body.get();
    if (!m_body)
    {
        return -1;
    }

    if (m_body->header_version() >= 2)
    {
        uint64_t tagCount;
        err = pd.getUVariant(tagCount);
        if (err != 0)
            return err;
    }

    return prepare_flexible_decoder(pd, *const_cast<protocol_body *>(m_body), version);
}

bool Request::is_flexible() const
{

    if (m_body)
    {
        auto f = dynamic_cast<const flexible_version *>(m_body);
        if (f != nullptr)
        {
            return f->is_flexible();
        }
    }
    return false;
}
coev::awaitable<int> decodeRequest(std::shared_ptr<Broker> &broker, Request &req, int &size)
{
    std::string lengthBytes;
    auto bytesRead = 4;
    int err = co_await broker->ReadFull(lengthBytes, bytesRead);
    if (err != 0)
    {
        size = bytesRead;
        co_return err;
    }
    size_t length = (static_cast<uint32_t>(lengthBytes[0]) << 24) | (static_cast<uint32_t>(lengthBytes[1]) << 16) |
                    (static_cast<uint32_t>(lengthBytes[2]) << 8) | static_cast<uint32_t>(lengthBytes[3]);
    if (length <= 4 || length > MaxRequestSize)
    {
        size = bytesRead;
        co_return -1;
    }

    std::string encodedReq;
    err = co_await broker->ReadFull(encodedReq, length);
    if (err != 0)
    {
        size = bytesRead + length;
        co_return -1;
    }
    bytesRead += length;

    err = ::decode(encodedReq, req);
    if (err != 0)
    {
        size = bytesRead;
        co_return err;
    }
    size = bytesRead;
    co_return 0;
}

std::shared_ptr<protocol_body> allocateBody(int16_t key, int16_t version)
{
    switch (key)
    {
    case apiKeyProduce:
        return std::make_shared<ProduceRequest>(version);
    case apiKeyFetch:
        return std::make_shared<FetchRequest>(version);
    case apiKeyListOffsets:
        return std::make_shared<OffsetRequest>(version);
    case apiKeyMetadata:
        return std::make_shared<MetadataRequest>(version);
    case apiKeyOffsetCommit:
        return std::make_shared<OffsetCommitRequest>(version);
    case apiKeyOffsetFetch:
        return std::make_shared<OffsetFetchRequest>(version);
    case apiKeyFindCoordinator:
        return std::make_shared<FindCoordinatorRequest>(version);
    case apiKeyJoinGroup:
        return std::make_shared<JoinGroupRequest>(version);
    case apiKeyHeartbeat:
        return std::make_shared<HeartbeatRequest>(version);
    case apiKeyLeaveGroup:
        return std::make_shared<LeaveGroupRequest>(version);
    case apiKeySyncGroup:
        return std::make_shared<SyncGroupRequest>(version);
    case apiKeyDescribeGroups:
        return std::make_shared<DescribeGroupsRequest>(version);
    case apiKeyListGroups:
        return std::make_shared<ListGroupsRequest>(version);
    case apiKeySaslHandshake:
        return std::make_shared<SaslHandshakeRequest>(version);
    case apiKeyApiVersions:
        return std::make_shared<ApiVersionsRequest>(version);
    case apiKeyCreateTopics:
        return std::make_shared<CreateTopicsRequest>(version);
    case apiKeyDeleteTopics:
        return std::make_shared<DeleteTopicsRequest>(version);
    case apiKeyDeleteRecords:
        return std::make_shared<DeleteRecordsRequest>(version);
    case apiKeyInitProducerId:
        return std::make_shared<InitProducerIDRequest>(version);
    case apiKeyAddPartitionsToTxn:
        return std::make_shared<AddPartitionsToTxnRequest>(version);
    case apiKeyAddOffsetsToTxn:
        return std::make_shared<AddOffsetsToTxnRequest>(version);
    case apiKeyEndTxn:
        return std::make_shared<EndTxnRequest>(version);
    case apiKeyTxnOffsetCommit:
        return std::make_shared<TxnOffsetCommitRequest>(version);
    case apiKeyDescribeAcls:
        return std::make_shared<DescribeAclsRequest>(static_cast<int>(version));
    case apiKeyCreateAcls:
        return std::make_shared<CreateAclsRequest>(version);
    case apiKeyDeleteAcls:
        return std::make_shared<DeleteAclsRequest>(static_cast<int>(version));
    case apiKeyDescribeConfigs:
        return std::make_shared<DescribeConfigsRequest>(version);
    case apiKeyAlterConfigs:
        return std::make_shared<AlterConfigsRequest>(version);
    case apiKeyDescribeLogDirs:
        return std::make_shared<DescribeLogDirsRequest>(version);
    case apiKeySASLAuth:
        return std::make_shared<SaslAuthenticateRequest>(version);
    case apiKeyCreatePartitions:
        return std::make_shared<CreatePartitionsRequest>(version);
    case apiKeyDeleteGroups:
        return std::make_shared<DeleteGroupsRequest>(version);
    case apiKeyElectLeaders:
        return std::make_shared<ElectLeadersRequest>(version);
    case apiKeyIncrementalAlterConfigs:
        return std::make_shared<IncrementalAlterConfigsRequest>(version);
    case apiKeyAlterPartitionReassignments:
        return std::make_shared<AlterPartitionReassignmentsRequest>(version);
    case apiKeyListPartitionReassignments:
        return std::make_shared<ListPartitionReassignmentsRequest>(version);
    case apiKeyOffsetDelete:
        return std::make_shared<DeleteOffsetsRequest>(version);
    case apiKeyDescribeClientQuotas:
        return std::make_shared<DescribeClientQuotasRequest>(version);
    case apiKeyAlterClientQuotas:
        return std::make_shared<AlterClientQuotasRequest>(version);
    case apiKeyDescribeUserScramCredentials:
        return std::make_shared<DescribeUserScramCredentialsRequest>(version);
    case apiKeyAlterUserScramCredentials:
        return std::make_shared<AlterUserScramCredentialsRequest>(version);
    default:
        return nullptr;
    }
}