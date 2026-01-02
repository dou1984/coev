#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <coev.h>
#include "config.h"
#include "client.h"
#include "broker.h"
#include "create_topics_request.h"
#include "metadata_response.h"
#include "list_partition_reassignments_request.h"
#include "acl_types.h"
#include "config_resource_type.h"
#include "acl_filter.h"
#include "acl_delete_response.h"
#include "offset_fetch_response.h"
#include "leave_group_response.h"
#include "sleep_for.h"
#include "topic_partition.h"

struct IClusterAdmin
{
    virtual ~IClusterAdmin() = default;
    virtual coev::awaitable<int> CreateTopic(const std::string &topic, const std::shared_ptr<TopicDetail> &detail, bool validateOnly) = 0;
    virtual coev::awaitable<int> ListTopics(std::map<std::string, TopicDetail> &out_topics) = 0;
    virtual coev::awaitable<int> DescribeTopics(const std::vector<std::string> &topics, std::vector<std::shared_ptr<TopicMetadata>> &metadata) = 0;
    virtual coev::awaitable<int> DeleteTopic(const std::string &topic) = 0;
    virtual coev::awaitable<int> CreatePartitions(const std::string &topic, int32_t count, const std::vector<std::vector<int32_t>> &assignment, bool validateOnly) = 0;
    virtual coev::awaitable<int> AlterPartitionReassignments(const std::string &topic, const std::vector<std::vector<int32_t>> &assignment) = 0;
    virtual coev::awaitable<int> ListPartitionReassignments(const std::string &topics, const std::vector<int32_t> &partitions, std::map<std::string, std::map<int32_t, std::shared_ptr<PartitionReplicaReassignmentsStatus>>> &topicStatus) = 0;
    virtual coev::awaitable<int> DeleteRecords(const std::string &topic, const std::map<int32_t, int64_t> &partitionOffsets) = 0;
    virtual coev::awaitable<int> DescribeConfig(const ConfigResource &resource, std::vector<std::shared_ptr<ConfigEntry>> &entries) = 0;
    virtual coev::awaitable<int> AlterConfig(ConfigResourceType resourceType, const std::string &name, const std::map<std::string, std::string> &entries, bool validateOnly) = 0;
    virtual coev::awaitable<int> IncrementalAlterConfig(ConfigResourceType resourceType, const std::string &name, const std::map<std::string, IncrementalAlterConfigsEntry> &entries, bool validateOnly) = 0;
    virtual coev::awaitable<int> CreateACL(const Resource &resource, const Acl &acl) = 0;
    virtual coev::awaitable<int> CreateACLs(const std::vector<std::shared_ptr<ResourceAcls>> &resourceACLs) = 0;
    virtual coev::awaitable<int> ListAcls(const AclFilter &filter, std::vector<std::shared_ptr<ResourceAcls>> &out_results) = 0;
    virtual coev::awaitable<int> DeleteACL(const AclFilter &filter, bool validateOnly, std::vector<std::shared_ptr<MatchingAcl>> &matchingAcls) = 0;
    virtual coev::awaitable<int> ElectLeaders(ElectionType electionType, const std::unordered_map<std::string, std::vector<int32_t>> &partitions, std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<PartitionResult>>> &results) = 0;
    virtual coev::awaitable<int> ListConsumerGroups(std::map<std::string, std::string> &groups) = 0;
    virtual coev::awaitable<int> DescribeConsumerGroups(const std::vector<std::string> &groups, std::vector<std::shared_ptr<GroupDescription>> &result) = 0;
    virtual coev::awaitable<int> ListConsumerGroupOffsets(const std::string &group, const std::map<std::string, std::vector<int32_t>> &topicPartitions, std::shared_ptr<OffsetFetchResponse> &response) = 0;
    virtual coev::awaitable<int> DeleteConsumerGroupOffset(const std::string &group, const std::string &topic, int32_t partition) = 0;
    virtual coev::awaitable<int> DeleteConsumerGroup(const std::string &group) = 0;
    virtual coev::awaitable<int> DescribeCluster(std::vector<std::shared_ptr<Broker>> &brokers, int32_t &controllerID) = 0;
    virtual coev::awaitable<int> DescribeLogDirs(const std::vector<int32_t> &brokers, std::map<int32_t, std::vector<DescribeLogDirsResponseDirMetadata>> &allLogDirs) = 0;
    virtual coev::awaitable<int> DescribeUserScramCredentials(const std::vector<std::string> &users, std::vector<std::shared_ptr<DescribeUserScramCredentialsResult>> &results) = 0;
    virtual coev::awaitable<int> DeleteUserScramCredentials(const std::vector<AlterUserScramCredentialsDelete> &delete_ops, std::vector<std::shared_ptr<AlterUserScramCredentialsResult>> &results) = 0;
    virtual coev::awaitable<int> UpsertUserScramCredentials(const std::vector<AlterUserScramCredentialsUpsert> &upsert_ops, std::vector<std::shared_ptr<AlterUserScramCredentialsResult>> &results) = 0;
    virtual coev::awaitable<int> AlterUserScramCredentials(const std::vector<AlterUserScramCredentialsUpsert> &u, const std::vector<AlterUserScramCredentialsDelete> &d, std::vector<std::shared_ptr<AlterUserScramCredentialsResult>> &out_results) = 0;
    virtual coev::awaitable<int> DescribeClientQuotas(const std::vector<QuotaFilterComponent> &components, bool strict, std::vector<DescribeClientQuotasEntry> &entries) = 0;
    virtual coev::awaitable<int> AlterClientQuotas(const std::vector<QuotaEntityComponent> &entity, const ClientQuotasOp &op, bool validateOnly) = 0;
    virtual coev::awaitable<int> GetController(std::shared_ptr<Broker> &broker) = 0;
    virtual coev::awaitable<int> GetCoordinator(const std::string &group, std::shared_ptr<Broker> &broker) = 0;
    virtual coev::awaitable<int> RemoveMemberFromConsumerGroup(const std::string &groupId, const std::vector<std::string> &groupInstanceIds, std::shared_ptr<LeaveGroupResponse> &response) = 0;
    virtual int Close() = 0;
};
struct ClusterAdmin : IClusterAdmin
{

    static std::shared_ptr<ClusterAdmin> Create(std::shared_ptr<IClient> client, std::shared_ptr<Config> conf);
    virtual ~ClusterAdmin() = default;
    int Close();
    coev::awaitable<int> GetController(std::shared_ptr<Broker> &out_broker);
    coev::awaitable<int> GetCoordinator(const std::string &group, std::shared_ptr<Broker> &out_broker);
    coev::awaitable<int> CreateTopic(const std::string &topic, const std::shared_ptr<TopicDetail> &detail, bool validateOnly);
    coev::awaitable<int> ListTopics(std::map<std::string, TopicDetail> &out_topics);
    coev::awaitable<int> DescribeTopics(const std::vector<std::string> &topics, std::vector<std::shared_ptr<TopicMetadata>> &out_metadata);
    coev::awaitable<int> DeleteTopic(const std::string &topic);
    coev::awaitable<int> CreatePartitions(const std::string &topic, int32_t count, const std::vector<std::vector<int32_t>> &assignment, bool validateOnly);
    coev::awaitable<int> AlterPartitionReassignments(const std::string &topic, const std::vector<std::vector<int32_t>> &assignment);
    coev::awaitable<int> ListPartitionReassignments(const std::string &topic, const std::vector<int32_t> &partitions, std::map<std::string, std::map<int32_t, std::shared_ptr<PartitionReplicaReassignmentsStatus>>> &out_topicStatus);
    coev::awaitable<int> DeleteRecords(const std::string &topic, const std::map<int32_t, int64_t> &partitionOffsets);
    coev::awaitable<int> DescribeConfig(const ConfigResource &resource, std::vector<std::shared_ptr<ConfigEntry>> &out_entries);
    coev::awaitable<int> AlterConfig(ConfigResourceType resourceType, const std::string &name, const std::map<std::string, std::string> &entries, bool validateOnly);
    coev::awaitable<int> IncrementalAlterConfig(ConfigResourceType resourceType, const std::string &name, const std::map<std::string, IncrementalAlterConfigsEntry> &entries, bool validateOnly);
    coev::awaitable<int> CreateACL(const Resource &resource, const Acl &acl);
    coev::awaitable<int> CreateACLs(const std::vector<std::shared_ptr<ResourceAcls>> &resourceACLs);
    coev::awaitable<int> ListAcls(const AclFilter &filter, std::vector<std::shared_ptr<ResourceAcls>> &out_results);
    coev::awaitable<int> DeleteACL(const AclFilter &filter, bool validateOnly, std::vector<std::shared_ptr<MatchingAcl>> &out_results);
    coev::awaitable<int> ElectLeaders(ElectionType electionType, const std::unordered_map<std::string, std::vector<int32_t>> &partitions, std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<PartitionResult>>> &out_results);
    coev::awaitable<int> ListConsumerGroups(std::map<std::string, std::string> &out_groups);
    coev::awaitable<int> DescribeConsumerGroups(const std::vector<std::string> &groups, std::vector<std::shared_ptr<GroupDescription>> &out_result);
    coev::awaitable<int> ListConsumerGroupOffsets(const std::string &group, const std::map<std::string, std::vector<int32_t>> &topicPartitions, std::shared_ptr<OffsetFetchResponse> &out_results);
    coev::awaitable<int> DeleteConsumerGroupOffset(const std::string &group, const std::string &topic, int32_t partition);
    coev::awaitable<int> DeleteConsumerGroup(const std::string &group);
    coev::awaitable<int> DescribeCluster(std::vector<std::shared_ptr<Broker>> &out_brokers, int32_t &out_controllerID);
    coev::awaitable<int> DescribeLogDirs(const std::vector<int32_t> &brokers, std::map<int32_t, std::vector<DescribeLogDirsResponseDirMetadata>> &out_allLogDirs);
    coev::awaitable<int> DescribeUserScramCredentials(const std::vector<std::string> &users, std::vector<std::shared_ptr<DescribeUserScramCredentialsResult>> &out_results);
    coev::awaitable<int> DeleteUserScramCredentials(const std::vector<AlterUserScramCredentialsDelete> &delete_ops, std::vector<std::shared_ptr<AlterUserScramCredentialsResult>> &out);
    coev::awaitable<int> UpsertUserScramCredentials(const std::vector<AlterUserScramCredentialsUpsert> &upsert_ops, std::vector<std::shared_ptr<AlterUserScramCredentialsResult>> &out);
    coev::awaitable<int> AlterUserScramCredentials(const std::vector<AlterUserScramCredentialsUpsert> &u, const std::vector<AlterUserScramCredentialsDelete> &d, std::vector<std::shared_ptr<AlterUserScramCredentialsResult>> &out_results);
    coev::awaitable<int> DescribeClientQuotas(const std::vector<QuotaFilterComponent> &components, bool strict, std::vector<DescribeClientQuotasEntry> &out);
    coev::awaitable<int> AlterClientQuotas(const std::vector<QuotaEntityComponent> &entity, const ClientQuotasOp &op, bool validateOnly);
    coev::awaitable<int> RemoveMemberFromConsumerGroup(const std::string &groupId, const std::vector<std::string> &groupInstanceIds, std::shared_ptr<LeaveGroupResponse> &out);

    ClusterAdmin(std::shared_ptr<IClient> client, std::shared_ptr<Config> conf);

    coev::awaitable<int> RetryOnError(std::function<bool(int)> retryable, std::function<coev::awaitable<int>()> fn);
    coev::awaitable<int> RefreshController(std::shared_ptr<Broker> &out);
    int FindBroker(int32_t id, std::shared_ptr<Broker> &out);
    int FindAnyBroker(std::shared_ptr<Broker> &out);

    std::shared_ptr<IClient> client_;
    std::shared_ptr<Config> conf_;
};