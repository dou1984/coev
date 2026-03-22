/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <coev/coev.h>
#include <coev_kafka/admin.h>
#include <coev_kafka/client.h>
#include <coev_kafka/config.h>
#include <coev_kafka/broker.h>

using namespace coev;
using namespace coev::kafka;

std::string test_host;
int test_port;
std::string test_topic;

void run_admin_test(const std::string &h, int p, const std::string &t);

int main(int argc, char *argv[])
{
    if (argc != 4) {
        LOG_ERR("Usage: admin_test <host> <port> <topic>");
        return 1;
    }
    test_host = argv[1];
    test_port = std::stoi(argv[2]);
    test_topic = argv[3];
    
    run_admin_test(test_host, test_port, test_topic);
    return 0;
}

void run_admin_test(const std::string &h, int p, const std::string &t)
{
    test_host = h;
    test_port = p;
    test_topic = t;

    runnable::instance()
        .start(
            []() -> awaitable<void>
            {
                std::vector<std::string> addrs = {test_host + ":" + std::to_string(test_port)};

                std::shared_ptr<Client> client;
                auto err = co_await NewClient(addrs, std::make_shared<Config>(), client);
                if (err)
                {
                    LOG_ERR("NewClient error: %d", err);
                    co_return;
                }

                auto admin = ClusterAdmin::Create(client, std::make_shared<Config>());
                if (!admin)
                {
                    LOG_ERR("Failed to create ClusterAdmin");
                    co_return;
                }

                LOG_DBG("=== Admin Test Started ===");

                // Test 1: DescribeCluster
                std::vector<std::shared_ptr<Broker>> brokers;
                int32_t controllerID;
                err = co_await admin->DescribeCluster(brokers, controllerID);
                LOG_DBG("[Test 1] DescribeCluster: %zu brokers, controller=%d, err=%d", brokers.size(), controllerID, err);

                // Test 2: ListTopics
                std::map<std::string, TopicDetail> topics;
                err = co_await admin->ListTopics(topics);
                LOG_DBG("[Test 2] ListTopics: %zu topics, err=%d", topics.size(), err);

                // Test 3: GetController
                std::shared_ptr<Broker> controller;
                err = co_await admin->GetController(controller);
                LOG_DBG("[Test 3] GetController: err=%d", err);

                // Test 4: RefreshController
                err = co_await admin->RefreshController(controller);
                LOG_DBG("[Test 4] RefreshController: err=%d", err);

                // Test 5: ListConsumerGroups
                std::map<std::string, std::string> groups;
                err = co_await admin->ListConsumerGroups(groups);
                LOG_DBG("[Test 5] ListConsumerGroups: %zu groups, err=%d", groups.size(), err);

                // Test 6: GetCoordinator
                std::shared_ptr<Broker> coordinator;
                err = co_await admin->GetCoordinator("test-group", coordinator);
                LOG_DBG("[Test 6] GetCoordinator: err=%d", err);

                // Test 7: DescribeTopics
                std::vector<std::string> topics_to_describe = {test_topic};
                std::vector<TopicMetadata> topic_metadata;
                err = co_await admin->DescribeTopics(topics_to_describe, topic_metadata);
                LOG_DBG("[Test 7] DescribeTopics: %zu topics, err=%d", topic_metadata.size(), err);

                // Test 8: DescribeConsumerGroups
                std::vector<std::string> groups_to_describe;
                if (!groups.empty()) {
                    groups_to_describe.push_back(groups.begin()->first);
                }
                std::vector<GroupDescription> group_descriptions;
                err = co_await admin->DescribeConsumerGroups(groups_to_describe, group_descriptions);
                LOG_DBG("[Test 8] DescribeConsumerGroups: %zu groups, err=%d", group_descriptions.size(), err);

                // Test 9: DescribeCluster (again to verify consistency)
                std::vector<std::shared_ptr<Broker>> brokers2;
                int32_t controllerID2;
                err = co_await admin->DescribeCluster(brokers2, controllerID2);
                LOG_DBG("[Test 9] DescribeCluster (again): %zu brokers, controller=%d, err=%d", brokers2.size(), controllerID2, err);

                // Test 10: DescribeLogDirs
                std::vector<int32_t> broker_ids = {1};
                std::map<int32_t, std::vector<DescribeLogDirsResponseDirMetadata>> log_dirs;
                err = co_await admin->DescribeLogDirs(broker_ids, log_dirs);
                LOG_DBG("[Test 10] DescribeLogDirs: %zu brokers, err=%d", log_dirs.size(), err);

                // Test 11: FindBroker
                std::shared_ptr<Broker> found_broker;
                err = admin->FindBroker(1, found_broker);
                LOG_DBG("[Test 11] FindBroker: err=%d", err);

                // Test 12: FindAnyBroker
                std::shared_ptr<Broker> any_broker;
                err = admin->FindAnyBroker(any_broker);
                LOG_DBG("[Test 12] FindAnyBroker: err=%d", err);

                // Test 13: ListConsumerGroupOffsets
                std::map<std::string, std::vector<int32_t>> topic_partitions1;
                topic_partitions1[test_topic] = {0};
                std::shared_ptr<OffsetFetchResponse> offsets_response1;
                err = co_await admin->ListConsumerGroupOffsets("test-group", topic_partitions1, offsets_response1);
                LOG_DBG("[Test 13] ListConsumerGroupOffsets: err=%d", err);

                // Test 14: DescribeConfig (query broker config)
                ConfigResource broker_resource(ConfigResourceType::BrokerResource, "1");
                std::vector<ConfigEntry> config_entries;
                err = co_await admin->DescribeConfig(broker_resource, config_entries);
                LOG_DBG("[Test 14] DescribeConfig (broker): %zu entries, err=%d", config_entries.size(), err);

                // Test 15: DescribeConfig (query topic config)
                ConfigResource topic_resource(ConfigResourceType::TopicResource, test_topic);
                std::vector<ConfigEntry> topic_config_entries;
                err = co_await admin->DescribeConfig(topic_resource, topic_config_entries);
                LOG_DBG("[Test 15] DescribeConfig (topic): %zu entries, err=%d", topic_config_entries.size(), err);

                // Test 16: ListAcls (query only, no modification)
                AclFilter acl_filter;
                std::vector<ResourceAcls> acls;
                err = co_await admin->ListAcls(acl_filter, acls);
                LOG_DBG("[Test 16] ListAcls: %zu acls, err=%d", acls.size(), err);

                // Test 17: DescribeClientQuotas (query only, no modification)
                std::vector<QuotaFilterComponent> quota_components;
                std::vector<DescribeClientQuotasEntry> quota_entries;
                err = co_await admin->DescribeClientQuotas(quota_components, false, quota_entries);
                LOG_DBG("[Test 17] DescribeClientQuotas: %zu quotas, err=%d", quota_entries.size(), err);

                // Test 18: ListPartitionReassignments (query only, no modification)
                std::map<std::string, std::map<int32_t, PartitionReplicaReassignmentsStatus>> reassignments;
                err = co_await admin->ListPartitionReassignments(test_topic, {}, reassignments);
                LOG_DBG("[Test 18] ListPartitionReassignments: err=%d", err);

                // Test 19: DescribeUserScramCredentials (query only, no modification)
                std::vector<std::string> users;
                std::vector<DescribeUserScramCredentialsResult> scram_results;
                err = co_await admin->DescribeUserScramCredentials(users, scram_results);
                LOG_DBG("[Test 19] DescribeUserScramCredentials: %zu results, err=%d", scram_results.size(), err);

                // Test 20: DescribeLogDirs (query only, no modification)
                std::vector<int32_t> broker_ids2 = {1};
                std::map<int32_t, std::vector<DescribeLogDirsResponseDirMetadata>> log_dirs2;
                err = co_await admin->DescribeLogDirs(broker_ids2, log_dirs2);
                LOG_DBG("[Test 20] DescribeLogDirs: %zu brokers, err=%d", log_dirs2.size(), err);

                // Test 21: CreateTopic (validate only)
                auto topic_detail = std::make_shared<TopicDetail>();
                topic_detail->m_num_partitions = 1;
                topic_detail->m_replication_factor = 1;
                err = co_await admin->CreateTopic("test-topic-temp", topic_detail, true); // validate only
                LOG_DBG("[Test 21] CreateTopic (validate): err=%d", err);

                // Test 22: DeleteTopic (validate only)
                err = co_await admin->DeleteTopic("test-topic-temp");
                LOG_DBG("[Test 22] DeleteTopic: err=%d", err);

                // Test 23: CreatePartitions (validate only)
                err = co_await admin->CreatePartitions(test_topic, 2, {}, true); // validate only
                LOG_DBG("[Test 23] CreatePartitions (validate): err=%d", err);

                // Test 24: DeleteRecords (validate only)
                std::map<int32_t, int64_t> partition_offsets;
                partition_offsets[0] = 0;
                err = co_await admin->DeleteRecords(test_topic, partition_offsets);
                LOG_DBG("[Test 24] DeleteRecords: err=%d", err);

                // Test 25: AlterConfig (validate only)
                std::map<std::string, std::string> alter_config_entries;
                alter_config_entries["retention.ms"] = "86400000";
                err = co_await admin->AlterConfig(ConfigResourceType::TopicResource, test_topic, alter_config_entries, true); // validate only
                LOG_DBG("[Test 25] AlterConfig (validate): err=%d", err);

                // Test 26: ElectLeaders (validate only)
                std::unordered_map<std::string, std::vector<int32_t>> partitions;
                partitions[test_topic] = {0};
                std::unordered_map<std::string, std::map<int32_t, PartitionResult>> election_results;
                err = co_await admin->ElectLeaders(ElectionType::Preferred, partitions, election_results);
                LOG_DBG("[Test 26] ElectLeaders: err=%d", err);

                // Test 27: DeleteConsumerGroupOffset
                err = co_await admin->DeleteConsumerGroupOffset("test-group", test_topic, 0);
                LOG_DBG("[Test 27] DeleteConsumerGroupOffset: err=%d", err);

                // Test 28: DeleteConsumerGroup
                err = co_await admin->DeleteConsumerGroup("test-group");
                LOG_DBG("[Test 28] DeleteConsumerGroup: err=%d", err);

                // Test 29: RemoveMemberFromConsumerGroup
                std::vector<std::string> member_ids;
                std::shared_ptr<LeaveGroupResponse> leave_response;
                err = co_await admin->RemoveMemberFromConsumerGroup("test-group", member_ids, leave_response);
                LOG_DBG("[Test 29] RemoveMemberFromConsumerGroup: err=%d", err);

                // Test 30: ListConsumerGroupOffsets
                std::map<std::string, std::vector<int32_t>> topic_partitions2;
                topic_partitions2[test_topic] = {0};
                std::shared_ptr<OffsetFetchResponse> offsets_response2;
                err = co_await admin->ListConsumerGroupOffsets("test-group", topic_partitions2, offsets_response2);
                LOG_DBG("[Test 30] ListConsumerGroupOffsets: err=%d", err);

                // Test 31: AlterPartitionReassignments (validate only)
                std::vector<std::vector<int32_t>> assignment;
                assignment.push_back({1}); // replica for partition 0
                err = co_await admin->AlterPartitionReassignments(test_topic, assignment);
                LOG_DBG("[Test 31] AlterPartitionReassignments: err=%d", err);

                // Test 32: DeleteUserScramCredentials
                std::vector<AlterUserScramCredentialsDelete> delete_ops;
                std::vector<AlterUserScramCredentialsResult> delete_results;
                err = co_await admin->DeleteUserScramCredentials(delete_ops, delete_results);
                LOG_DBG("[Test 32] DeleteUserScramCredentials: err=%d", err);

                // Test 33: UpsertUserScramCredentials
                std::vector<AlterUserScramCredentialsUpsert> upsert_ops;
                std::vector<AlterUserScramCredentialsResult> upsert_results;
                err = co_await admin->UpsertUserScramCredentials(upsert_ops, upsert_results);
                LOG_DBG("[Test 33] UpsertUserScramCredentials: err=%d", err);

                // Test 34: AlterUserScramCredentials
                std::vector<AlterUserScramCredentialsResult> alter_results;
                err = co_await admin->AlterUserScramCredentials(upsert_ops, delete_ops, alter_results);
                LOG_DBG("[Test 34] AlterUserScramCredentials: err=%d", err);

                // Test 35: Close
                err = admin->Close();
                LOG_DBG("[Test 35] Close: err=%d", err);

                // Test 36: IncrementalAlterConfig (validate only)
                std::map<std::string, IncrementalAlterConfigsEntry> incremental_entries;
                IncrementalAlterConfigsEntry entry;
                entry.m_operation = IncrementalAlterConfigsOperationSet;
                entry.m_value = "86400000";
                incremental_entries["retention.ms"] = entry;
                err = co_await admin->IncrementalAlterConfig(ConfigResourceType::TopicResource, test_topic, incremental_entries, true); // validate only
                LOG_DBG("[Test 36] IncrementalAlterConfig (validate): err=%d", err);

                // Test 37: CreateACL (validate only)
                Resource acl_resource;
                acl_resource.m_resource_type = AclResourceTypeTopic;
                acl_resource.m_resource_name = test_topic;
                acl_resource.m_resource_pattern_type = AclResourcePatternTypeLiteral;
                Acl acl;
                acl.m_principal = "User:test";
                acl.m_host = "*";
                acl.m_operation = AclOperationRead;
                acl.m_permission_type = AclPermissionTypeAllow;
                err = co_await admin->CreateACL(acl_resource, acl);
                LOG_DBG("[Test 37] CreateACL: err=%d", err);

                // Test 38: CreateACLs (validate only)
                std::vector<ResourceAcls> resource_acls;
                ResourceAcls resource_acl;
                resource_acl.m_resource = acl_resource;
                resource_acl.m_acls.push_back(acl);
                resource_acls.push_back(resource_acl);
                err = co_await admin->CreateACLs(resource_acls);
                LOG_DBG("[Test 38] CreateACLs: err=%d", err);

                // Test 39: DeleteACL (validate only)
                AclFilter acl_filter2;
                acl_filter2.m_resource_type = AclResourceTypeTopic;
                acl_filter2.m_resource_name = test_topic;
                acl_filter2.m_resource_pattern_type_filter = AclResourcePatternTypeLiteral;
                acl_filter2.m_principal = "User:test";
                acl_filter2.m_host = "*";
                acl_filter2.m_operation = AclOperationRead;
                acl_filter2.m_permission_type = AclPermissionTypeAllow;
                std::vector<MatchingAcl> matching_acls;
                err = co_await admin->DeleteACL(acl_filter2, true, matching_acls); // validate only
                LOG_DBG("[Test 39] DeleteACL (validate): err=%d", err);

                // Test 40: AlterClientQuotas (validate only)
                std::vector<QuotaEntityComponent> quota_entity;
                QuotaEntityComponent entity_component;
                entity_component.m_entity_type = "user";
                entity_component.m_name = "test";
                entity_component.m_match_type = QuotaMatchExact;
                quota_entity.push_back(entity_component);
                ClientQuotasOp quota_op;
                quota_op.m_key = "producer_byte_rate";
                quota_op.m_value = 1024 * 1024; // 1MB/s
                err = co_await admin->AlterClientQuotas(quota_entity, quota_op, true); // validate only
                LOG_DBG("[Test 40] AlterClientQuotas (validate): err=%d", err);

                LOG_DBG("=== Admin Test Completed ===");
                co_return;
            }).wait();
}
