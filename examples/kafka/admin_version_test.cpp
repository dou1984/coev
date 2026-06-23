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
#include <coev_kafka/version.h>
#include "supported_versions.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <vector>

using namespace coev;
using namespace coev::kafka;

std::string test_host;
int test_port;
std::string test_topic;

struct AdminOperationResult
{
    std::string operation_name;
    bool success;
    int error_code;
    std::string error_msg;
};

struct AdminVersionTestResult
{
    KafkaVersion version;
    std::vector<AdminOperationResult> operations;
    int total_ops;
    int passed_ops;
};

static awaitable<AdminOperationResult> test_admin_operation(
    const std::string &name,
    std::function<awaitable<int>()> op)
{
    AdminOperationResult result;
    result.operation_name = name;
    result.success = false;
    result.error_code = 0;

    try
    {
        int err = co_await op();
        result.error_code = err;
        result.success = (err == 0);
        if (err != 0)
        {
            result.error_msg = "Error code: " + std::to_string(err);
        }
    }
    catch (const std::exception &e)
    {
        result.error_msg = std::string("Exception: ") + e.what();
    }
    catch (...)
    {
        result.error_msg = "Unknown exception";
    }

    co_return result;
}

static awaitable<AdminVersionTestResult> test_admin_version(
    const KafkaVersion &ver,
    const std::string &host,
    int port,
    const std::string &topic)
{
    AdminVersionTestResult result;
    result.version = ver;
    result.total_ops = 0;
    result.passed_ops = 0;

    try
    {
        // 创建配置
        std::shared_ptr<Config> conf = std::make_shared<Config>();
        conf->Version = ver;

        // 创建客户端
        std::vector<std::string> addrs = {host + ":" + std::to_string(port)};
        std::shared_ptr<Client> client;
        int err = co_await NewClient(addrs, conf, client);
        if (err)
        {
            AdminOperationResult op_result;
            op_result.operation_name = "NewClient";
            op_result.success = false;
            op_result.error_code = err;
            op_result.error_msg = "Failed to create client";
            result.operations.push_back(op_result);
            result.total_ops = 1;
            co_return result;
        }

        // 创建 Admin
        auto admin = ClusterAdmin::Create(client, conf);
        if (!admin)
        {
            AdminOperationResult op_result;
            op_result.operation_name = "CreateAdmin";
            op_result.success = false;
            op_result.error_msg = "Failed to create admin";
            result.operations.push_back(op_result);
            result.total_ops = 1;
            co_return result;
        }

        // 测试所有 AdminClient 操作
        auto ops = std::vector<std::pair<std::string, std::function<awaitable<int>()>>>{
            // 1. DescribeCluster
            {"DescribeCluster", [&]() -> awaitable<int> {
                std::vector<std::shared_ptr<Broker>> brokers;
                int32_t controllerID;
                co_return co_await admin->DescribeCluster(brokers, controllerID);
            }},
            // 2. GetController
            {"GetController", [&]() -> awaitable<int> {
                std::shared_ptr<Broker> controller;
                co_return co_await admin->GetController(controller);
            }},
            // 3. ListTopics
            {"ListTopics", [&]() -> awaitable<int> {
                std::map<std::string, TopicDetail> topics;
                co_return co_await admin->ListTopics(topics);
            }},
            // 4. DescribeTopics
            {"DescribeTopics", [&]() -> awaitable<int> {
                std::vector<std::string> topics = {topic};
                std::vector<TopicMetadata> metadata;
                co_return co_await admin->DescribeTopics(topics, metadata);
            }},
            // 5. CreateTopic (validate only)
            {"CreateTopic", [&]() -> awaitable<int> {
                auto detail = std::make_shared<TopicDetail>();
                detail->m_num_partitions = 1;
                detail->m_replication_factor = 1;
                co_return co_await admin->CreateTopic(topic + "-test", detail, true);
            }},
            // 6. DeleteTopic (validate only)
            {"DeleteTopic", [&]() -> awaitable<int> {
                co_return co_await admin->DeleteTopic(topic + "-test");
            }},
            // 7. CreatePartitions (validate only)
            {"CreatePartitions", [&]() -> awaitable<int> {
                co_return co_await admin->CreatePartitions(topic, 2, {}, true);
            }},
            // 8. DescribeConfig (topic)
            {"DescribeConfig", [&]() -> awaitable<int> {
                ConfigResource resource(TopicResource, topic);
                std::vector<ConfigEntry> entries;
                co_return co_await admin->DescribeConfig(resource, entries);
            }},
            // 9. AlterConfig (validate only)
            {"AlterConfig", [&]() -> awaitable<int> {
                std::map<std::string, std::string> entries;
                entries["retention.ms"] = "86400000";
                co_return co_await admin->AlterConfig(TopicResource, topic, entries, true);
            }},
            // 10. IncrementalAlterConfig (validate only)
            {"IncrementalAlterConfig", [&]() -> awaitable<int> {
                std::map<std::string, IncrementalAlterConfigsEntry> entries;
                IncrementalAlterConfigsEntry entry;
                entry.m_operation = IncrementalAlterConfigsOperationSet;
                entry.m_value = "86400000";
                entries["retention.ms"] = entry;
                co_return co_await admin->IncrementalAlterConfig(TopicResource, topic, entries, true);
            }},
            // 11. DescribeLogDirs
            {"DescribeLogDirs", [&]() -> awaitable<int> {
                std::vector<int32_t> broker_ids = {1};
                std::map<int32_t, std::vector<DescribeLogDirsResponseDirMetadata>> log_dirs;
                co_return co_await admin->DescribeLogDirs(broker_ids, log_dirs);
            }},
            // 12. ListConsumerGroups
            {"ListConsumerGroups", [&]() -> awaitable<int> {
                std::map<std::string, std::string> groups;
                co_return co_await admin->ListConsumerGroups(groups);
            }},
            // 13. DescribeConsumerGroups
            {"DescribeConsumerGroups", [&]() -> awaitable<int> {
                std::vector<std::string> groups = {"test-group"};
                std::vector<GroupDescription> descriptions;
                co_return co_await admin->DescribeConsumerGroups(groups, descriptions);
            }},
            // 14. ListConsumerGroupOffsets
            {"ListConsumerGroupOffsets", [&]() -> awaitable<int> {
                std::map<std::string, std::vector<int32_t>> topic_partitions;
                topic_partitions[topic] = {0};
                std::shared_ptr<OffsetFetchResponse> response;
                co_return co_await admin->ListConsumerGroupOffsets("test-group", topic_partitions, response);
            }},
            // 15. DeleteConsumerGroupOffset
            {"DeleteConsumerGroupOffset", [&]() -> awaitable<int> {
                co_return co_await admin->DeleteConsumerGroupOffset("test-group", topic, 0);
            }},
            // 16. DeleteConsumerGroup
            {"DeleteConsumerGroup", [&]() -> awaitable<int> {
                co_return co_await admin->DeleteConsumerGroup("test-group");
            }},
            // 17. ListAcls
            {"ListAcls", [&]() -> awaitable<int> {
                AclFilter filter;
                std::vector<ResourceAcls> acls;
                co_return co_await admin->ListAcls(filter, acls);
            }},
            // 18. CreateACL (validate only)
            {"CreateACL", [&]() -> awaitable<int> {
                Resource resource;
                resource.m_resource_type = AclResourceTypeTopic;
                resource.m_resource_name = topic;
                resource.m_resource_pattern_type = AclResourcePatternTypeLiteral;
                Acl acl;
                acl.m_principal = "User:test";
                acl.m_host = "*";
                acl.m_operation = AclOperationRead;
                acl.m_permission_type = AclPermissionTypeAllow;
                co_return co_await admin->CreateACL(resource, acl);
            }},
            // 19. DeleteACL (validate only)
            {"DeleteACL", [&]() -> awaitable<int> {
                AclFilter filter;
                filter.m_resource_type = AclResourceTypeTopic;
                filter.m_resource_name = topic;
                filter.m_resource_pattern_type_filter = AclResourcePatternTypeLiteral;
                filter.m_principal = "User:test";
                filter.m_host = "*";
                filter.m_operation = AclOperationRead;
                filter.m_permission_type = AclPermissionTypeAllow;
                std::vector<MatchingAcl> matching;
                co_return co_await admin->DeleteACL(filter, true, matching);
            }},
            // 20. DescribeClientQuotas
            {"DescribeClientQuotas", [&]() -> awaitable<int> {
                std::vector<QuotaFilterComponent> components;
                std::vector<DescribeClientQuotasEntry> entries;
                co_return co_await admin->DescribeClientQuotas(components, false, entries);
            }},
            // 21. AlterClientQuotas (validate only)
            {"AlterClientQuotas", [&]() -> awaitable<int> {
                std::vector<QuotaEntityComponent> entity;
                QuotaEntityComponent entity_component;
                entity_component.m_entity_type = "user";
                entity_component.m_name = "test";
                entity_component.m_match_type = QuotaMatchExact;
                entity.push_back(entity_component);
                ClientQuotasOp op;
                op.m_key = "producer_byte_rate";
                op.m_value = 1024 * 1024;
                co_return co_await admin->AlterClientQuotas(entity, op, true);
            }},
            // 22. DescribeUserScramCredentials
            {"DescribeUserScramCredentials", [&]() -> awaitable<int> {
                std::vector<std::string> users;
                std::vector<DescribeUserScramCredentialsResult> results;
                co_return co_await admin->DescribeUserScramCredentials(users, results);
            }},
            // 23. UpsertUserScramCredentials
            {"UpsertUserScramCredentials", [&]() -> awaitable<int> {
                std::vector<AlterUserScramCredentialsUpsert> upserts;
                std::vector<AlterUserScramCredentialsResult> results;
                co_return co_await admin->UpsertUserScramCredentials(upserts, results);
            }},
            // 24. DeleteUserScramCredentials
            {"DeleteUserScramCredentials", [&]() -> awaitable<int> {
                std::vector<AlterUserScramCredentialsDelete> deletes;
                std::vector<AlterUserScramCredentialsResult> results;
                co_return co_await admin->DeleteUserScramCredentials(deletes, results);
            }},
            // 25. ElectLeaders (validate only)
            {"ElectLeaders", [&]() -> awaitable<int> {
                std::unordered_map<std::string, std::vector<int32_t>> partitions;
                partitions[topic] = {0};
                std::unordered_map<std::string, std::map<int32_t, PartitionResult>> results;
                co_return co_await admin->ElectLeaders(ElectionType::Preferred, partitions, results);
            }},
            // 26. AlterPartitionReassignments
            {"AlterPartitionReassignments", [&]() -> awaitable<int> {
                std::vector<std::vector<int32_t>> assignment;
                assignment.push_back({1});
                co_return co_await admin->AlterPartitionReassignments(topic, assignment);
            }},
            // 27. ListPartitionReassignments
            {"ListPartitionReassignments", [&]() -> awaitable<int> {
                std::map<std::string, std::map<int32_t, PartitionReplicaReassignmentsStatus>> reassignments;
                co_return co_await admin->ListPartitionReassignments(topic, {}, reassignments);
            }},
            // 28. DeleteRecords
            {"DeleteRecords", [&]() -> awaitable<int> {
                std::map<int32_t, int64_t> partition_offsets;
                partition_offsets[0] = 0;
                co_return co_await admin->DeleteRecords(topic, partition_offsets);
            }},
            // 29. RemoveMemberFromConsumerGroup
            {"RemoveMemberFromConsumerGroup", [&]() -> awaitable<int> {
                std::vector<std::string> member_ids;
                std::shared_ptr<LeaveGroupResponse> response;
                co_return co_await admin->RemoveMemberFromConsumerGroup("test-group", member_ids, response);
            }},
        };

        for (const auto &[op_name, op_func] : ops)
        {
            auto op_result = co_await test_admin_operation(op_name, op_func);
            result.operations.push_back(op_result);
            result.total_ops++;
            if (op_result.success)
            {
                result.passed_ops++;
            }
        }

        admin->Close();
    }
    catch (const std::exception &e)
    {
        AdminOperationResult op_result;
        op_result.operation_name = "Exception";
        op_result.success = false;
        op_result.error_msg = std::string("Exception: ") + e.what();
        result.operations.push_back(op_result);
        result.total_ops++;
    }
    catch (...)
    {
        AdminOperationResult op_result;
        op_result.operation_name = "Exception";
        op_result.success = false;
        op_result.error_msg = "Unknown exception";
        result.operations.push_back(op_result);
        result.total_ops++;
    }

    co_return result;
}

void run_admin_version_test()
{
    runnable::instance()
        .start(
            []() -> awaitable<void>
            {
                std::cout << "=== Admin Version Test ===" << std::endl;
                std::cout << "Broker: " << test_host << ":" << test_port << std::endl;
                std::cout << "Topic: " << test_topic << std::endl;
                std::cout << "Total versions to test: " << SupportedVersions().size() << std::endl;
                std::cout << "==========================" << std::endl;

                std::vector<AdminVersionTestResult> all_results;

                for (size_t i = 0; i < SupportedVersions().size(); ++i)
                {
                    const auto &ver = SupportedVersions()[i];
                    std::cout << "\n[" << i + 1 << "/" << SupportedVersions().size()
                              << "] Testing version: " << ver.String() << std::endl;

                    auto result = co_await test_admin_version(ver, test_host, test_port, test_topic);
                    all_results.push_back(result);

                    std::cout << "  Operations: " << result.passed_ops << "/" << result.total_ops << " passed" << std::endl;
                    for (const auto &op : result.operations)
                    {
                        std::cout << "    " << std::setw(35) << std::left << op.operation_name
                                  << " - " << (op.success ? "PASS" : "FAIL");
                        if (!op.success && !op.error_msg.empty())
                        {
                            std::cout << " (" << op.error_msg << ")";
                        }
                        std::cout << std::endl;
                    }
                }

                // 打印总结
                std::cout << "\n=== Test Summary ===" << std::endl;
                std::cout << "Total versions tested: " << all_results.size() << std::endl;

                int total_pass = 0;
                int total_tests = 0;

                for (const auto &result : all_results)
                {
                    total_tests += result.total_ops;
                    total_pass += result.passed_ops;
                }

                std::cout << "Overall: " << total_pass << "/" << total_tests
                          << " operations passed (" << std::fixed << std::setprecision(1)
                          << (total_tests > 0 ? 100.0 * total_pass / total_tests : 0) << "%)" << std::endl;

                co_return;
            })
        .end();
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        std::cout << "Usage: " << argv[0] << " <host> <port> <topic>" << std::endl;
        return -1;
    }

    test_host = argv[1];
    test_port = std::stoi(argv[2]);
    test_topic = argv[3];

    run_admin_version_test();

    return 0;
}
