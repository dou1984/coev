/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <coev/coev.h>
#include <coev_kafka/client.h>
#include <coev_kafka/config.h>
#include <coev_kafka/broker.h>
#include <coev_kafka/request.h>
#include <coev_kafka/protocol_body.h>
#include <coev_kafka/produce_request.h>
#include <coev_kafka/fetch_request.h>
#include <coev_kafka/offset_request.h>
#include <coev_kafka/metadata_request.h>
#include <coev_kafka/offset_commit_request.h>
#include <coev_kafka/offset_fetch_request.h>
#include <coev_kafka/find_coordinator_request.h>
#include <coev_kafka/join_group_request.h>
#include <coev_kafka/heartbeat_request.h>
#include <coev_kafka/leave_group_request.h>
#include <coev_kafka/sync_group_request.h>
#include <coev_kafka/describe_groups_request.h>
#include <coev_kafka/list_groups_request.h>
#include <coev_kafka/sasl_handshake_request.h>
#include <coev_kafka/sasl_authenticate_request.h>
#include <coev_kafka/api_versions_request.h>
#include <coev_kafka/create_topics_request.h>
#include <coev_kafka/delete_topics_request.h>
#include <coev_kafka/delete_records_request.h>
#include <coev_kafka/init_producer_id_request.h>
#include <coev_kafka/add_partitions_to_txn_request.h>
#include <coev_kafka/add_offsets_to_txn_request.h>
#include <coev_kafka/end_txn_request.h>
#include <coev_kafka/txn_offset_commit_request.h>
#include <coev_kafka/acl_describe_request.h>
#include <coev_kafka/acl_create_request.h>
#include <coev_kafka/acl_delete_request.h>
#include <coev_kafka/describe_configs_request.h>
#include <coev_kafka/alter_configs_request.h>
#include <coev_kafka/describe_log_dirs_request.h>
#include <coev_kafka/create_partitions_request.h>
#include <coev_kafka/delete_groups_request.h>
#include <coev_kafka/elect_leaders_request.h>
#include <coev_kafka/incremental_alter_configs_request.h>
#include <coev_kafka/alter_partition_reassignments_request.h>
#include <coev_kafka/list_partition_reassignments_request.h>
#include <coev_kafka/delete_offsets_request.h>
#include <coev_kafka/describe_client_quotas_request.h>
#include <coev_kafka/alter_client_quotas_request.h>
#include <coev_kafka/describe_user_scram_credentials_request.h>
#include <coev_kafka/alter_user_scram_credentials_request.h>
#include <coev_kafka/consumer_metadata_request.h>
#include <coev_kafka/version.h>
#include "supported_versions.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <memory>

using namespace coev;
using namespace coev::kafka;

std::string test_host;
int test_port;
std::string test_topic;

struct MessageTestResult
{
    std::string message_name;
    int api_key;
    bool encode_ok;
    bool decode_ok;
    int error_code;
    std::string error_msg;
};

struct VersionTestResult
{
    KafkaVersion version;
    std::vector<MessageTestResult> messages;
    int total_msgs;
    int passed_msgs;
};

// 测试消息编解码的辅助函数
template<typename RequestType>
static MessageTestResult test_message_encode_decode(
    const std::string &name,
    int api_key,
    int16_t version)
{
    MessageTestResult result;
    result.message_name = name;
    result.api_key = api_key;
    result.encode_ok = false;
    result.decode_ok = false;
    result.error_code = 0;

    try
    {
        // 创建请求
        RequestType request;
        request.set_version(version);

        // 编码请求 - 使用自由函数
        std::string encoded;
        int err = coev::kafka::encode(request, encoded);
        if (err != 0)
        {
            result.error_code = err;
            result.error_msg = "Encode failed";
            return result;
        }
        result.encode_ok = true;

        // 解码请求（验证）- 使用带版本的自由函数
        RequestType decoded_request;
        err = coev::kafka::decode_version(encoded, decoded_request, version);
        if (err != 0)
        {
            result.error_code = err;
            result.error_msg = "Decode request failed";
            // 调试输出：打印详细错误信息
            std::cout << "  [DEBUG] " << name << " v" << version << " err=" << err
                      << " encoded_size=" << encoded.size() << " hex:";
            for (auto c : encoded)
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << (int)(unsigned char)c;
            std::cout << std::dec << std::endl;
            return result;
        }
        result.decode_ok = true;

        return result;
    }
    catch (const std::exception &e)
    {
        result.error_msg = std::string("Exception: ") + e.what();
        return result;
    }
    catch (...)
    {
        result.error_msg = "Unknown exception";
        return result;
    }
}

static VersionTestResult test_all_messages_version(
    const KafkaVersion &ver,
    const std::string &host,
    int port)
{
    VersionTestResult result;
    result.version = ver;
    result.total_msgs = 0;
    result.passed_msgs = 0;

    int16_t version = 0; // 使用版本 0 进行基础测试

    // 测试所有消息类型
    auto test_cases = std::vector<std::pair<std::string, std::function<MessageTestResult()>>>{
        {"Produce", [&]() { return test_message_encode_decode<ProduceRequest>("Produce", 0, version); }},
        {"Fetch", [&]() { return test_message_encode_decode<FetchRequest>("Fetch", 1, version); }},
        {"ListOffsets", [&]() { return test_message_encode_decode<OffsetRequest>("ListOffsets", 2, version); }},
        {"Metadata", [&]() { return test_message_encode_decode<MetadataRequest>("Metadata", 3, version); }},
        {"OffsetCommit", [&]() { return test_message_encode_decode<OffsetCommitRequest>("OffsetCommit", 8, version); }},
        {"OffsetFetch", [&]() { return test_message_encode_decode<OffsetFetchRequest>("OffsetFetch", 9, version); }},
        {"FindCoordinator", [&]() { return test_message_encode_decode<FindCoordinatorRequest>("FindCoordinator", 10, version); }},
        {"JoinGroup", [&]() { return test_message_encode_decode<JoinGroupRequest>("JoinGroup", 11, version); }},
        {"Heartbeat", [&]() { return test_message_encode_decode<HeartbeatRequest>("Heartbeat", 12, version); }},
        {"LeaveGroup", [&]() { return test_message_encode_decode<LeaveGroupRequest>("LeaveGroup", 13, version); }},
        {"SyncGroup", [&]() { return test_message_encode_decode<SyncGroupRequest>("SyncGroup", 14, version); }},
        {"DescribeGroups", [&]() { return test_message_encode_decode<DescribeGroupsRequest>("DescribeGroups", 15, version); }},
        {"ListGroups", [&]() { return test_message_encode_decode<ListGroupsRequest>("ListGroups", 16, version); }},
        {"SaslHandshake", [&]() { return test_message_encode_decode<SaslHandshakeRequest>("SaslHandshake", 17, version); }},
        {"SaslAuthenticate", [&]() { return test_message_encode_decode<SaslAuthenticateRequest>("SaslAuthenticate", 36, version); }},
        {"ApiVersions", [&]() { return test_message_encode_decode<ApiVersionsRequest>("ApiVersions", 18, version); }},
        {"CreateTopics", [&]() { return test_message_encode_decode<CreateTopicsRequest>("CreateTopics", 19, version); }},
        {"DeleteTopics", [&]() { return test_message_encode_decode<DeleteTopicsRequest>("DeleteTopics", 20, version); }},
        {"DeleteRecords", [&]() { return test_message_encode_decode<DeleteRecordsRequest>("DeleteRecords", 21, version); }},
        {"InitProducerId", [&]() { return test_message_encode_decode<InitProducerIDRequest>("InitProducerId", 22, version); }},
        {"AddPartitionsToTxn", [&]() { return test_message_encode_decode<AddPartitionsToTxnRequest>("AddPartitionsToTxn", 24, version); }},
        {"AddOffsetsToTxn", [&]() { return test_message_encode_decode<AddOffsetsToTxnRequest>("AddOffsetsToTxn", 25, version); }},
        {"EndTxn", [&]() { return test_message_encode_decode<EndTxnRequest>("EndTxn", 26, version); }},
        {"TxnOffsetCommit", [&]() { return test_message_encode_decode<TxnOffsetCommitRequest>("TxnOffsetCommit", 28, version); }},
        {"DescribeAcls", [&]() { return test_message_encode_decode<DescribeAclsRequest>("DescribeAcls", 29, version); }},
        {"CreateAcls", [&]() { return test_message_encode_decode<CreateAclsRequest>("CreateAcls", 30, version); }},
        {"DeleteAcls", [&]() { return test_message_encode_decode<DeleteAclsRequest>("DeleteAcls", 31, version); }},
        {"DescribeConfigs", [&]() { return test_message_encode_decode<DescribeConfigsRequest>("DescribeConfigs", 32, version); }},
        {"AlterConfigs", [&]() { return test_message_encode_decode<AlterConfigsRequest>("AlterConfigs", 33, version); }},
        {"DescribeLogDirs", [&]() { return test_message_encode_decode<DescribeLogDirsRequest>("DescribeLogDirs", 35, version); }},
        {"CreatePartitions", [&]() { return test_message_encode_decode<CreatePartitionsRequest>("CreatePartitions", 37, version); }},
        {"DeleteGroups", [&]() { return test_message_encode_decode<DeleteGroupsRequest>("DeleteGroups", 42, version); }},
        {"ElectLeaders", [&]() { return test_message_encode_decode<ElectLeadersRequest>("ElectLeaders", 43, version); }},
        {"IncrementalAlterConfigs", [&]() { return test_message_encode_decode<IncrementalAlterConfigsRequest>("IncrementalAlterConfigs", 44, version); }},
        {"AlterPartitionReassignments", [&]() { return test_message_encode_decode<AlterPartitionReassignmentsRequest>("AlterPartitionReassignments", 45, version); }},
        {"ListPartitionReassignments", [&]() { return test_message_encode_decode<ListPartitionReassignmentsRequest>("ListPartitionReassignments", 46, version); }},
        {"OffsetDelete", [&]() { return test_message_encode_decode<DeleteOffsetsRequest>("OffsetDelete", 47, version); }},
        {"DescribeClientQuotas", [&]() { return test_message_encode_decode<DescribeClientQuotasRequest>("DescribeClientQuotas", 48, version); }},
        {"AlterClientQuotas", [&]() { return test_message_encode_decode<AlterClientQuotasRequest>("AlterClientQuotas", 49, version); }},
        {"DescribeUserScramCredentials", [&]() { return test_message_encode_decode<DescribeUserScramCredentialsRequest>("DescribeUserScramCredentials", 50, version); }},
        {"AlterUserScramCredentials", [&]() { return test_message_encode_decode<AlterUserScramCredentialsRequest>("AlterUserScramCredentials", 51, version); }},
        {"ConsumerMetadata", [&]() { return test_message_encode_decode<ConsumerMetadataRequest>("ConsumerMetadata", 42, version); }},
    };

    for (const auto &[msg_name, test_func] : test_cases)
    {
        auto msg_result = test_func();
        result.messages.push_back(msg_result);
        result.total_msgs++;
        if (msg_result.encode_ok && msg_result.decode_ok)
        {
            result.passed_msgs++;
        }
    }

    return result;
}

void run_all_messages_version_test()
{
    runnable::instance()
        .start(
            []() -> awaitable<void>
            {
                std::cout << "=== All Messages Version Test ===" << std::endl;
                std::cout << "Broker: " << test_host << ":" << test_port << std::endl;
                std::cout << "Total versions to test: " << SupportedVersions().size() << std::endl;
                std::cout << "Total message types: 41" << std::endl;
                std::cout << "=================================" << std::endl;

                std::vector<VersionTestResult> all_results;

                for (size_t i = 0; i < SupportedVersions().size(); ++i)
                {
                    const auto &ver = SupportedVersions()[i];
                    std::cout << "\n[" << i + 1 << "/" << SupportedVersions().size()
                              << "] Testing version: " << ver.String() << std::endl;

                    auto result = test_all_messages_version(ver, test_host, test_port);
                    all_results.push_back(result);

                    std::cout << "  Messages: " << result.passed_msgs << "/" << result.total_msgs << " passed" << std::endl;

                    // 只显示失败的消息
                    for (const auto &msg : result.messages)
                    {
                        if (!msg.encode_ok || !msg.decode_ok)
                        {
                            std::cout << "    [FAIL] " << std::setw(35) << std::left << msg.message_name
                                      << " - Encode: " << (msg.encode_ok ? "OK" : "FAIL")
                                      << ", Decode: " << (msg.decode_ok ? "OK" : "FAIL");
                            if (!msg.error_msg.empty())
                            {
                                std::cout << " (" << msg.error_msg << ")";
                            }
                            std::cout << std::endl;
                        }
                    }
                }

                // 打印总结
                std::cout << "\n=== Test Summary ===" << std::endl;
                std::cout << "Total versions tested: " << all_results.size() << std::endl;

                int total_pass = 0;
                int total_tests = 0;

                for (const auto &result : all_results)
                {
                    total_tests += result.total_msgs;
                    total_pass += result.passed_msgs;
                }

                std::cout << "Overall: " << total_pass << "/" << total_tests
                          << " messages passed (" << std::fixed << std::setprecision(1)
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

    run_all_messages_version_test();

    return 0;
}
