/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <coev/coev.h>
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

namespace coev::kafka
{
    struct ClusterAdmin
    {

        static std::shared_ptr<ClusterAdmin> Create(std::shared_ptr<Client> client, std::shared_ptr<Config> conf);

        ClusterAdmin(std::shared_ptr<Client> client, std::shared_ptr<Config> conf);
        virtual ~ClusterAdmin() = default;
        int Close();

        awaitable<int> GetController(std::shared_ptr<Broker> &out_broker);
        awaitable<int> GetCoordinator(const std::string &group, std::shared_ptr<Broker> &out_broker);
        awaitable<int> CreateTopic(const std::string &topic, const std::shared_ptr<TopicDetail> &detail, bool validate_only);
        awaitable<int> ListTopics(std::map<std::string, TopicDetail> &out);
        awaitable<int> DescribeTopics(const std::vector<std::string> &topics, std::vector<TopicMetadata> &out_metadata);
        awaitable<int> DeleteTopic(const std::string &topic);
        awaitable<int> CreatePartitions(const std::string &topic, int32_t count, const std::vector<std::vector<int32_t>> &assignment, bool validate_only);
        awaitable<int> AlterPartitionReassignments(const std::string &topic, const std::vector<std::vector<int32_t>> &assignment);
        awaitable<int> ListPartitionReassignments(const std::string &topic, const std::vector<int32_t> &partitions, std::map<std::string, std::map<int32_t, PartitionReplicaReassignmentsStatus>> &out);
        awaitable<int> DeleteRecords(const std::string &topic, const std::map<int32_t, int64_t> &partition_offsets);
        awaitable<int> DescribeConfig(const ConfigResource &resource, std::vector<ConfigEntry> &out);
        awaitable<int> AlterConfig(ConfigResourceType resourceType, const std::string &name, const std::map<std::string, std::string> &entries, bool validate_only);
        awaitable<int> IncrementalAlterConfig(ConfigResourceType resourceType, const std::string &name, const std::map<std::string, IncrementalAlterConfigsEntry> &entries, bool validate_only);
        awaitable<int> CreateACL(const Resource &resource, const Acl &acl);
        awaitable<int> CreateACLs(const std::vector<ResourceAcls> &resource_acls);
        awaitable<int> ListAcls(const AclFilter &filter, std::vector<ResourceAcls> &out);
        awaitable<int> DeleteACL(const AclFilter &filter, bool validate_only, std::vector<MatchingAcl> &out);
        awaitable<int> ElectLeaders(ElectionType electionType, const std::unordered_map<std::string, std::vector<int32_t>> &partitions, std::unordered_map<std::string, std::map<int32_t, PartitionResult>> &out);
        awaitable<int> ListConsumerGroups(std::map<std::string, std::string> &out_groups);
        awaitable<int> DescribeConsumerGroups(const std::vector<std::string> &groups, std::vector<GroupDescription> &out_result);
        awaitable<int> ListConsumerGroupOffsets(const std::string &group, const std::map<std::string, std::vector<int32_t>> &topic_partitions, std::shared_ptr<OffsetFetchResponse> &out);
        awaitable<int> DeleteConsumerGroupOffset(const std::string &group, const std::string &topic, int32_t partition);
        awaitable<int> DeleteConsumerGroup(const std::string &group);
        awaitable<int> DescribeCluster(std::vector<std::shared_ptr<Broker>> &out, int32_t &out_controllerID);
        awaitable<int> DescribeLogDirs(const std::vector<int32_t> &brokers, std::map<int32_t, std::vector<DescribeLogDirsResponseDirMetadata>> &out);
        awaitable<int> DescribeUserScramCredentials(const std::vector<std::string> &users, std::vector<DescribeUserScramCredentialsResult> &out);
        awaitable<int> DeleteUserScramCredentials(const std::vector<AlterUserScramCredentialsDelete> &delete_ops, std::vector<AlterUserScramCredentialsResult> &out);
        awaitable<int> UpsertUserScramCredentials(const std::vector<AlterUserScramCredentialsUpsert> &upsert_ops, std::vector<AlterUserScramCredentialsResult> &out);
        awaitable<int> AlterUserScramCredentials(const std::vector<AlterUserScramCredentialsUpsert> &u, const std::vector<AlterUserScramCredentialsDelete> &d, std::vector<AlterUserScramCredentialsResult> &out);
        awaitable<int> DescribeClientQuotas(const std::vector<QuotaFilterComponent> &components, bool strict, std::vector<DescribeClientQuotasEntry> &out);
        awaitable<int> AlterClientQuotas(const std::vector<QuotaEntityComponent> &entity, const ClientQuotasOp &op, bool validate_only);
        awaitable<int> RemoveMemberFromConsumerGroup(const std::string &groupId, const std::vector<std::string> &group_instance_ids, std::shared_ptr<LeaveGroupResponse> &out);
        awaitable<int> RefreshController(std::shared_ptr<Broker> &out);

        int FindBroker(int32_t id, std::shared_ptr<Broker> &out);
        int FindAnyBroker(std::shared_ptr<Broker> &out);

    private:
        awaitable<int> _CreateTopic(const std::string &topic, std::shared_ptr<CreateTopicsRequest> request);
        awaitable<int> _DescribeTopics(const std::vector<std::string> &topics, std::vector<TopicMetadata> &out);
        awaitable<int> _DeleteTopic(const std::string &topic, std::shared_ptr<DeleteTopicsRequest> request);
        awaitable<int> _AlterPartitionReassignments(std::shared_ptr<AlterPartitionReassignmentsRequest> request);
        awaitable<int> _CreatePartitions(const std::string &topic, std::shared_ptr<CreatePartitionsRequest> request);
        awaitable<int> _ListPartitionReassignments(std::shared_ptr<ListPartitionReassignmentsRequest> request, std::map<std::string, std::map<int32_t, PartitionReplicaReassignmentsStatus>> &);
        awaitable<int> _DescribeCluster(std::vector<std::shared_ptr<Broker>> &, int32_t &);
        awaitable<int> _ListConsumerGroups(std::shared_ptr<Broker>, std::list<int> &, std::map<std::string, std::string> &);
        awaitable<int> _ListConsumerGroupOffsets(const std::string &group, std::shared_ptr<OffsetFetchRequest> request, std::shared_ptr<OffsetFetchResponse> &out);
        awaitable<int> _DeleteConsumerGroupOffset(std::shared_ptr<DeleteOffsetsRequest> request, const std::string &group, const std::string &topic, int32_t partition);
        awaitable<int> _DeleteConsumerGroup(std::shared_ptr<DeleteGroupsRequest> request, const std::string &group);
        awaitable<int> _AlterUserScramCredentials(std::shared_ptr<AlterUserScramCredentialsRequest> req, ResponsePromise<AlterUserScramCredentialsResponse> &rsp);
        awaitable<int> _ElectLeaders(std::shared_ptr<ElectLeadersRequest> request, ResponsePromise<ElectLeadersResponse> &res);
        awaitable<int> _RemoveMemberFromConsumerGroup(std::shared_ptr<LeaveGroupRequest> request, std::shared_ptr<LeaveGroupResponse> &response, const std::string &groupId);

        std::shared_ptr<Client> m_client;
        std::shared_ptr<Config> m_conf;

        template <class Func, class... Args>
        awaitable<int> RetryOnError(std::function<bool(int)> retryable, const Func &func, Args &&...args)
        {
            int attempts = m_conf->Admin.Retry.Max + 1;
            while (true)
            {
                int err = co_await (this->*func)(std::forward<Args>(args)...);
                attempts--;
                if (err == 0 || attempts <= 0 || !retryable(err))
                {
                    co_return err;
                }
                co_await sleep_for(m_conf->Admin.Retry.Backoff);
            }
        }
    };

} // namespace coev::kafka