#include <random>
#include <thread>
#include <chrono>

#include "admin.h"
#include "errors.h"
#include "create_topics_request.h"
#include "delete_topics_request.h"
#include "create_partitions_request.h"
#include "alter_partition_reassignments_request.h"
#include "list_partition_reassignments_request.h"
#include "delete_records_request.h"
#include "describe_configs_request.h"
#include "alter_configs_request.h"
#include "incremental_alter_configs_request.h"
#include "acl_create_request.h"
#include "acl_describe_request.h"
#include "acl_delete_request.h"
#include "elect_leaders_request.h"
#include "describe_groups_request.h"
#include "list_groups_request.h"
#include "offset_fetch_request.h"
#include "delete_offsets_request.h"
#include "delete_groups_request.h"
#include "describe_log_dirs_request.h"
#include "describe_user_scram_credentials_request.h"
#include "alter_user_scram_credentials_request.h"
#include "describe_client_quotas_request.h"
#include "alter_client_quotas_request.h"
#include "leave_group_request.h"
#include "metadata_request.h"
#include "errors.h"

static bool isRetriableControllerError(int err)
{
    return err == ErrNotController || err == static_cast<int>(ErrIOEOF);
}

static bool isRetriableGroupCoordinatorError(int err)
{
    return err == ErrNotCoordinatorForConsumer ||
           err == ErrConsumerCoordinatorNotAvailable ||
           err == static_cast<int>(ErrIOEOF);
}

static bool dependsOnSpecificNode(const ConfigResource &resource)
{
    return (resource.m_type == BrokerResource && !resource.m_name.empty()) ||
           resource.m_type == BrokerLoggerResource;
}

std::shared_ptr<ClusterAdmin> ClusterAdmin::Create(std::shared_ptr<Client> client, std::shared_ptr<Config> conf)
{
    if (client == nullptr || conf == nullptr) {
        return nullptr;
    }
    return std::make_shared<ClusterAdmin>(client, conf);
}

ClusterAdmin::ClusterAdmin(std::shared_ptr<Client> client, std::shared_ptr<Config> conf)
    : m_client(client), m_conf(conf) {}

int ClusterAdmin::Close()
{
    return m_client->Close();
}

coev::awaitable<int> ClusterAdmin::GetController(std::shared_ptr<Broker> &out)
{
    return m_client->Controller(out);
}

coev::awaitable<int> ClusterAdmin::GetCoordinator(const std::string &group, std::shared_ptr<Broker> &out)
{
    return m_client->GetCoordinator(group, out);
}
coev::awaitable<int> ClusterAdmin::RefreshController(std::shared_ptr<Broker> &out)
{
    return m_client->RefreshController(out);
}

int ClusterAdmin::FindBroker(int32_t id, std::shared_ptr<Broker> &out_broker)
{
    auto brokers = m_client->Brokers();
    for (auto &broker : brokers)
    {
        if (broker->ID() == id)
        {
            out_broker = broker;
            return 0;
        }
    }
    return ErrBrokerNotFound;
}

int ClusterAdmin::FindAnyBroker(std::shared_ptr<Broker> &out_broker)
{
    auto brokers = m_client->Brokers();
    if (!brokers.empty())
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, static_cast<int>(brokers.size()) - 1);
        out_broker = brokers[dis(gen)];
        return 0;
    }
    return ErrBrokerNotFound;
}
coev::awaitable<int> ClusterAdmin::_CreateTopic(const std::string &topic, std::shared_ptr<CreateTopicsRequest> request)
{
    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<CreateTopicsResponse> rsp;
    err = co_await broker->CreateTopics(request, rsp);
    if (err != 0)
    {
        co_return err;
    }
    auto it = rsp.m_response->m_topic_errors.find(topic);
    if (it == rsp.m_response->m_topic_errors.end())
    {
        co_return ErrIncompleteResponse;
    }
    if (it->second->m_err != ErrNoError)
    {
        if (isRetriableControllerError(it->second->m_err))
        {
            std::shared_ptr<Broker> dummy;
            RefreshController(dummy);
        }
        co_return it->second->m_err;
    }
    co_return 0;
}
coev::awaitable<int> ClusterAdmin::CreateTopic(const std::string &topic, const std::shared_ptr<TopicDetail> &detail, bool validate_only)
{
    if (topic.empty())
    {
        co_return ErrInvalidTopic;
    }
    if (detail == nullptr)
    {
        co_return ErrTopicNoDetails;
    }
    std::map<std::string, std::shared_ptr<TopicDetail>> topicDetails;
    topicDetails[topic] = detail;
    auto request = std::make_shared<CreateTopicsRequest>(m_conf->Version, m_conf->Admin.Timeout.count(), validate_only, topicDetails);
    co_return co_await RetryOnError(isRetriableControllerError, &ClusterAdmin::_CreateTopic, topic, request);
}

coev::awaitable<int> ClusterAdmin::_DescribeTopics(const std::vector<std::string> &topics, std::vector<TopicMetadata> &out)
{
    std::shared_ptr<Broker> controller;
    auto err = co_await GetController(controller);
    if (err != 0)
    {
        co_return err;
    }
    auto request = std::make_shared<MetadataRequest>(m_conf->Version, topics);
    ResponsePromise<MetadataResponse> response;
    err = co_await controller->GetMetadata(request, response);
    if (isRetriableControllerError(err))
    {
        std::shared_ptr<Broker> dummy;
        co_await RefreshController(dummy);
    }
    if (err != 0)
    {
        co_return err;
    }
    out = std::move(response.m_response->m_topics);
    co_return 0;
}
coev::awaitable<int> ClusterAdmin::DescribeTopics(const std::vector<std::string> &topics, std::vector<TopicMetadata> &out)
{
    co_return co_await RetryOnError(isRetriableControllerError, &ClusterAdmin::_DescribeTopics, topics, out);
}

coev::awaitable<int> ClusterAdmin::_DescribeCluster(std::vector<std::shared_ptr<Broker>> &out, int32_t &out_controller_id)
{
    std::shared_ptr<Broker> controller;
    int err = co_await GetController(controller);
    if (err != 0)
    {
        co_return err;
    }
    auto request = std::make_shared<MetadataRequest>(m_conf->Version, std::vector<std::string>{});
    ResponsePromise<MetadataResponse> response;
    err = co_await controller->GetMetadata(request, response);
    if (isRetriableControllerError(err))
    {
        std::shared_ptr<Broker> dummy;
        RefreshController(dummy);
    }
    if (err != 0)
    {
        co_return err;
    }
    out = std::move(response.m_response->m_brokers);
    out_controller_id = response.m_response->m_controller_id;
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DescribeCluster(std::vector<std::shared_ptr<Broker>> &brokers, int32_t &controller_id)
{
    int err = co_await RetryOnError(isRetriableControllerError, &ClusterAdmin::_DescribeCluster, brokers, controller_id);
    if (err != 0)
    {
        brokers.clear();
        controller_id = 0;
    }
    co_return err;
}

coev::awaitable<int> ClusterAdmin::ListTopics(std::map<std::string, TopicDetail> &topics_details_map)
{
    std::shared_ptr<Broker> broker;
    int err = FindAnyBroker(broker);
    if (err != 0)
    {
        co_return err;
    }
    err = co_await broker->Open(m_conf);
    auto request = std::make_shared<MetadataRequest>(m_conf->Version, std::vector<std::string>{});
    ResponsePromise<MetadataResponse> response;
    err = co_await broker->GetMetadata(request, response);
    if (err != 0)
    {
        co_return err;
    }

    std::vector<ConfigResource> describe_configs_resources;
    for (auto &topic : response.m_response->m_topics)
    {
        auto &details = topics_details_map[topic.m_name];
        details.m_num_partitions = static_cast<int32_t>(topic.m_partitions.size());
        if (!topic.m_partitions.empty())
        {
            details.m_replica_assignment.clear();
            for (auto &partition : topic.m_partitions)
            {
                details.m_replica_assignment[partition.m_id] = partition.m_replicas;
            }
            details.m_replication_factor = static_cast<int16_t>(topic.m_partitions[0].m_replicas.size());
        }

        describe_configs_resources.emplace_back(TopicResource, topic.m_name);
    }

    auto describe_configs_request = std::make_shared<DescribeConfigsRequest>();
    describe_configs_request->m_resources = std::move(describe_configs_resources);
    if (m_conf->Version.IsAtLeast(V1_1_0_0))
    {
        describe_configs_request->m_version = 1;
    }
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        describe_configs_request->m_version = 2;
    }

    ResponsePromise<DescribeConfigsResponse> describeConfigsResp;
    err = co_await broker->DescribeConfigs(describe_configs_request, describeConfigsResp);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &resource : describeConfigsResp.m_response->m_resources)
    {
        auto it = topics_details_map.find(resource.m_name);
        if (it == topics_details_map.end())
            continue;
        auto &topic_details = it->second;
        topic_details.m_config_entries.clear();
        for (auto &entry : resource.m_configs)
        {
            if (entry.m_default || entry.m_sensitive)
                continue;
            topic_details.m_config_entries[entry.m_name] = entry.m_value;
        }
    }

    co_return 0;
}

coev::awaitable<int> ClusterAdmin::_DeleteTopic(const std::string &topic, std::shared_ptr<DeleteTopicsRequest> request)
{
    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<DeleteTopicsResponse> rsp;
    err = co_await broker->DeleteTopics(request, rsp);
    if (err != 0)
    {
        co_return err;
    }
    auto it = rsp.m_response->m_topic_error_codes.find(topic);
    if (it == rsp.m_response->m_topic_error_codes.end())
    {
        co_return ErrIncompleteResponse;
    }
    if (it->second != ErrNoError)
    {
        if (it->second == ErrNotController)
        {
            std::shared_ptr<Broker> dummy;
            RefreshController(dummy);
        }
        co_return it->second;
    }
    co_return 0;
}
coev::awaitable<int> ClusterAdmin::DeleteTopic(const std::string &topic)
{
    if (topic.empty())
    {
        co_return ErrInvalidTopic;
    }
    auto request = std::make_shared<DeleteTopicsRequest>(m_conf->Version, std::vector<std::string>{topic}, m_conf->Admin.Timeout.count());
    co_return co_await RetryOnError(isRetriableControllerError, &ClusterAdmin::_DeleteTopic, topic, request);
}

coev::awaitable<int> ClusterAdmin::_CreatePartitions(const std::string &topic, std::shared_ptr<CreatePartitionsRequest> request)
{
    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<CreatePartitionsResponse> rsp;
    err = co_await broker->CreatePartitions(request, rsp);
    if (err != 0)
    {
        co_return err;
    }
    auto it = rsp.m_response->m_topic_partition_errors.find(topic);
    if (it == rsp.m_response->m_topic_partition_errors.end())
    {
        co_return ErrIncompleteResponse;
    }
    if (it->second.m_err != ErrNoError)
    {
        if (it->second.m_err == ErrNotController)
        {
            std::shared_ptr<Broker> dummy;
            RefreshController(dummy);
        }
        co_return it->second.m_err;
    }
    co_return 0;
}
coev::awaitable<int> ClusterAdmin::CreatePartitions(const std::string &topic, int32_t count, const std::vector<std::vector<int32_t>> &assignment, bool validate_only)
{
    if (topic.empty())
    {
        co_return ErrInvalidTopic;
    }
    auto tp = std::make_shared<TopicPartition>();
    tp->m_count = count;
    tp->m_assignment = assignment;

    auto request = std::make_shared<CreatePartitionsRequest>();
    request->m_topic_partitions[topic] = tp;
    request->m_timeout = m_conf->Admin.Timeout;
    request->m_validate_only = validate_only;
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    co_return co_await RetryOnError(isRetriableControllerError, &ClusterAdmin::_CreatePartitions, topic, request);
}

coev::awaitable<int> ClusterAdmin::_AlterPartitionReassignments(std::shared_ptr<AlterPartitionReassignmentsRequest> request)
{
    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<AlterPartitionReassignmentsResponse> response;
    err = co_await broker->AlterPartitionReassignments(request, response);
    std::vector<int> errs;
    if (err != 0)
    {
        errs.push_back(err);
    }
    else
    {
        if (response.m_response->m_code > 0)
        {
            errs.push_back(response.m_response->m_code);
        }
        for (auto &[topic, partitions] : response.m_response->m_errors)
        {
            for (auto &[partition, error_block] : partitions)
            {
                if (error_block.m_code != ErrNoError)
                {
                    errs.push_back(error_block.m_code);
                }
            }
        }
    }
    if (!errs.empty())
    {
        co_return ErrReassignPartitions;
    }
    co_return 0;
}
coev::awaitable<int> ClusterAdmin::AlterPartitionReassignments(const std::string &topic, const std::vector<std::vector<int32_t>> &assignment)
{
    if (topic.empty())
    {
        co_return ErrInvalidTopic;
    }
    auto request = std::make_shared<AlterPartitionReassignmentsRequest>();
    request->m_timeout = std::chrono::milliseconds(60000);
    request->m_version = 0;
    for (size_t i = 0; i < assignment.size(); ++i)
    {
        request->add_block(topic, static_cast<int32_t>(i), assignment[i]);
    }

    co_return co_await RetryOnError(isRetriableControllerError, &ClusterAdmin::_AlterPartitionReassignments, request);
}

coev::awaitable<int> ClusterAdmin::_ListPartitionReassignments(std::shared_ptr<ListPartitionReassignmentsRequest> request, std::map<std::string, std::map<int32_t, PartitionReplicaReassignmentsStatus>> &output)
{
    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    err = co_await broker->Open(m_client->GetConfig());
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<ListPartitionReassignmentsResponse> response;
    err = co_await broker->ListPartitionReassignments(request, response);
    if (isRetriableControllerError(err))
    {
        std::shared_ptr<Broker> dummy;
        err = co_await RefreshController(dummy);
        if (err != 0)
        {
            co_return err;
        }
    }
    if (err == 0)
    {
        output = std::move(response.m_response->m_topic_status);
    }
    co_return err;
}
coev::awaitable<int> ClusterAdmin::ListPartitionReassignments(const std::string &topic, const std::vector<int32_t> &partitions, std::map<std::string, std::map<int32_t, PartitionReplicaReassignmentsStatus>> &output)
{
    if (topic.empty())
    {
        co_return ErrInvalidTopic;
    }
    auto request = std::make_shared<ListPartitionReassignmentsRequest>();
    request->m_timeout = std::chrono::milliseconds(60000);
    request->m_version = 0;
    request->add_block(topic, partitions);
    int err = co_await RetryOnError(isRetriableControllerError, &ClusterAdmin::_ListPartitionReassignments, request, output);
    if (err != 0)
    {
        output.clear();
    }
    co_return err;
}

coev::awaitable<int> ClusterAdmin::DeleteRecords(const std::string &topic, const std::map<int32_t, int64_t> &partition_offsets)
{
    if (topic.empty())
    {
        co_return ErrInvalidTopic;
    }
    std::vector<int> errs;
    std::map<std::shared_ptr<Broker>, std::vector<int32_t>> partitionPerBroker;
    for (auto &[partition, _] : partition_offsets)
    {
        std::shared_ptr<Broker> broker;
        int err = co_await m_client->Leader(topic, partition, broker);
        if (err != 0)
        {
            errs.push_back(err);
            continue;
        }
        partitionPerBroker[broker].push_back(partition);
    }

    for (auto &[broker, partitions] : partitionPerBroker)
    {

        std::unordered_map<int32_t, int64_t> records_to_delete;
        for (int32_t p : partitions)
        {
            records_to_delete[p] = partition_offsets.at(p);
        }
        std::unordered_map<std::string, std::shared_ptr<DeleteRecordsRequestTopic>> topics;
        auto topic_data = std::make_shared<DeleteRecordsRequestTopic>();
        topic_data->m_partition_offsets = std::move(records_to_delete);
        topics[topic] = topic_data;

        auto request = std::make_shared<DeleteRecordsRequest>();
        request->m_topics = std::move(topics);
        request->m_timeout = m_conf->Admin.Timeout;
        if (m_conf->Version.IsAtLeast(V2_0_0_0))
        {
            request->m_version = 1;
        }

        ResponsePromise<DeleteRecordsResponse> response;
        int err = co_await broker->DeleteRecords(request, response);
        if (err != 0)
        {
            errs.emplace_back(err);
            continue;
        }

        auto it = response.m_response->m_topics.find(topic);
        if (it == response.m_response->m_topics.end())
        {
            errs.emplace_back(ErrIncompleteResponse);
            continue;
        }

        for (auto &[_, part] : it->second->m_partitions)
        {
            if (part->m_err != ErrNoError)
            {
                errs.emplace_back(part->m_err);
            }
        }
    }

    if (!errs.empty())
    {
        co_return ErrDeleteRecords;
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DescribeConfig(const ConfigResource &cfg_resource, std::vector<ConfigEntry> &out)
{

    auto request = std::make_shared<DescribeConfigsRequest>();
    if (m_conf->Version.IsAtLeast(V1_1_0_0))
    {
        request->m_version = 1;
    }
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 2;
    }
    request->m_resources.emplace_back(cfg_resource);

    std::shared_ptr<Broker> broker;
    int err;
    if (dependsOnSpecificNode(cfg_resource))
    {
        int32_t id = static_cast<int32_t>(std::stol(cfg_resource.m_name));
        err = FindBroker(id, broker);
        if (err != 0)
        {
            co_return err;
        }
    }
    else
    {
        err = FindAnyBroker(broker);
    }
    if (err != 0)
    {
        co_return err;
    }
    err = co_await broker->Open(m_client->GetConfig());
    if (err != 0)
    {
        co_return err;
    }

    ResponsePromise<DescribeConfigsResponse> response;
    err = co_await broker->DescribeConfigs(request, response);
    if (err != 0)
    {
        co_return err;
    }

    out.clear();
    for (auto &resource : response.m_response->m_resources)
    {
        if (resource.m_name == cfg_resource.m_name)
        {
            if (resource.m_error_code != 0)
            {
                co_return resource.m_error_code;
            }

            out.insert(out.end(), resource.m_configs.begin(), resource.m_configs.end());
        }
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::AlterConfig(ConfigResourceType resource_type, const std::string &name,
                                               const std::map<std::string, std::string> &entries, bool validate_only)
{
    std::vector<AlterConfigsResource> resources;
    AlterConfigsResource res;
    res.m_type = resource_type;
    res.m_name = name;
    res.m_config_entries = entries;
    resources.push_back(res);

    auto request = std::make_shared<AlterConfigsRequest>();
    request->m_resources = resources;
    request->m_validate_only = validate_only;
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }
    int err;
    std::shared_ptr<Broker> broker;
    ConfigResource tmp(resource_type, name);
    if (dependsOnSpecificNode(tmp))
    {

        int32_t id = static_cast<int32_t>(std::stol(name));
        err = FindBroker(id, broker);
        if (err != 0)
        {
            co_return err;
        }
    }
    else
    {
        err = FindAnyBroker(broker);
        if (err != 0)
        {
            co_return err;
        }
    }

    err = co_await broker->Open(m_client->GetConfig());
    if (err != 0)
    {
        co_return err;
    }

    ResponsePromise<AlterConfigsResponse> response;
    err = co_await broker->AlterConfigs(request, response);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &resource : response.m_response->m_resources)
    {
        if (resource.m_name == name)
        {
            if (resource.m_error_code != 0)
            {
                co_return resource.m_error_code;
            }
        }
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::IncrementalAlterConfig(ConfigResourceType resource_type, const std::string &name, const std::map<std::string, IncrementalAlterConfigsEntry> &entries, bool validate_only)
{

    auto request = std::make_shared<IncrementalAlterConfigsRequest>();
    request->m_resources.emplace_back(resource_type, name, entries);
    request->m_validate_only = validate_only;
    if (m_conf->Version.IsAtLeast(V2_4_0_0))
    {
        request->m_version = 1;
    }

    std::shared_ptr<Broker> broker;
    int err;
    ConfigResource tmp(resource_type, name);
    if (dependsOnSpecificNode(tmp))
    {
        int32_t id = static_cast<int32_t>(std::stol(name));
        err = FindBroker(id, broker);
        if (err != 0)
        {
            co_return err;
        }
    }
    else
    {
        err = FindAnyBroker(broker);
    }
    if (err != 0)
    {
        co_return err;
    }
    err = co_await broker->Open(m_client->GetConfig());

    ResponsePromise<IncrementalAlterConfigsResponse> response;
    err = co_await broker->IncrementalAlterConfigs(request, response);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &resource : response.m_response->m_resources)
    {
        if (resource.m_name == name)
        {
            if (resource.m_error_code != ErrNoError)
            {
                int err = resource.m_error_code;
                if (!resource.m_error_msg.empty())
                {
                }
                co_return err;
            }
        }
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::CreateACL(const Resource &resource, const Acl &acl)
{
    auto request = std::make_shared<CreateAclsRequest>();
    request->m_acl_creations.emplace_back(resource, acl);
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<CreateAclsResponse> rsp;
    co_return co_await broker->CreateAcls(request, rsp);
}

coev::awaitable<int> ClusterAdmin::CreateACLs(const std::vector<ResourceAcls> &resource_acks)
{
    auto request = std::make_shared<CreateAclsRequest>();
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    for (auto &resource_acl : resource_acks)
    {
        for (auto &acl : resource_acl.m_acls)
        {
            request->m_acl_creations.emplace_back(resource_acl.m_resource, acl);
        }
    }

    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<CreateAclsResponse> response;
    co_return co_await broker->CreateAcls(request, response);
}

coev::awaitable<int> ClusterAdmin::ListAcls(const AclFilter &filter, std::vector<ResourceAcls> &out_results)
{
    auto request = std::make_shared<DescribeAclsRequest>();
    request->m_filter = filter;
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<DescribeAclsResponse> response;
    err = co_await broker->DescribeAcls(request, response);
    if (err != 0)
    {
        co_return err;
    }

    out_results = std::move(response.m_response->m_resource_acls);
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DeleteACL(const AclFilter &filter, bool validate_only, std::vector<MatchingAcl> &out)
{
    auto request = std::make_shared<DeleteAclsRequest>();
    request->m_filters = {filter};
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<DeleteAclsResponse> rsp;
    err = co_await broker->DeleteAcls(request, rsp);
    if (err != 0)
    {
        co_return err;
    }

    out.clear();
    for (auto &fr : rsp.m_response->m_filter_responses)
    {
        out.insert(out.end(), fr.m_matching_acls.begin(), fr.m_matching_acls.end());
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::_ElectLeaders(std::shared_ptr<ElectLeadersRequest> request, ResponsePromise<ElectLeadersResponse> &res)
{
    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    err = co_await broker->Open(m_client->GetConfig());
    if (err != 0)
    {
        co_return err;
    }
    err = co_await broker->ElectLeaders(request, res);
    if (err != 0)
    {
        co_return err;
    }
    if (res.m_response->m_error_code != ErrNoError)
    {
        if (isRetriableControllerError(res.m_response->m_error_code))
        {
            std::shared_ptr<Broker> dummy;
            RefreshController(dummy);
        }
        co_return res.m_response->m_error_code;
    }
    co_return 0;
}
coev::awaitable<int> ClusterAdmin::ElectLeaders(ElectionType electionType, const std::unordered_map<std::string, std::vector<int32_t>> &partitions, std::unordered_map<std::string, std::map<int32_t, PartitionResult>> &out)
{
    auto request = std::make_shared<ElectLeadersRequest>();
    request->m_type = electionType;
    request->m_topic_partitions = partitions;
    request->m_timeout = std::chrono::milliseconds(60000);
    if (m_conf->Version.IsAtLeast(V2_4_0_0))
    {
        request->m_version = 2;
    }
    else if (m_conf->Version.IsAtLeast(V0_11_0_0))
    {
        request->m_version = 1;
    }

    ResponsePromise<ElectLeadersResponse> res;
    int err = co_await RetryOnError(isRetriableControllerError, &ClusterAdmin::_ElectLeaders, request, res);
    if (err != 0)
    {
        out.clear();
        co_return err;
    }
    out = std::move(res.m_response->m_replica_election_results);
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DescribeConsumerGroups(const std::vector<std::string> &groups, std::vector<GroupDescription> &out)
{
    std::map<std::shared_ptr<Broker>, std::vector<std::string>> groups_per_broker;
    for (auto &group : groups)
    {
        std::shared_ptr<Broker> coordinator;
        int err = co_await m_client->GetCoordinator(group, coordinator);
        if (err != 0)
        {
            co_return err;
        }
        groups_per_broker[coordinator].push_back(group);
    }

    out.clear();
    for (auto &[broker, broker_groups] : groups_per_broker)
    {

        auto describe_request = std::make_shared<DescribeGroupsRequest>();
        describe_request->m_groups = broker_groups;
        if (m_conf->Version.IsAtLeast(V2_4_0_0))
        {
            describe_request->m_version = 5;
        }
        else if (m_conf->Version.IsAtLeast(V2_3_0_0))
        {
            describe_request->m_version = 3;
        }
        else if (m_conf->Version.IsAtLeast(V2_0_0_0))
        {
            describe_request->m_version = 2;
        }
        else if (m_conf->Version.IsAtLeast(V1_1_0_0))
        {
            describe_request->m_version = 1;
        }

        ResponsePromise<DescribeGroupsResponse> response;
        int err = co_await broker->DescribeGroups(describe_request, response);
        if (err != 0)
        {
            co_return err;
        }
        for (auto &group_desc : response.m_response->m_groups)
        {
            out.push_back(group_desc);
        }
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::ListConsumerGroups(std::map<std::string, std::string> &out)
{
    auto brokers = m_client->Brokers();

    std::list<int> err_chan;
    coev::co_task task;
    for (auto &broker : brokers)
    {
        task << _ListConsumerGroups(broker, err_chan, out);
    }
    co_await task.wait_all();

    co_return 0;
}
coev::awaitable<int> ClusterAdmin::_ListConsumerGroups(std::shared_ptr<Broker> broker, std::list<int> &err_chan, std::map<std::string, std::string> &out)
{
    broker->Open(m_conf);
    auto request = std::make_shared<ListGroupsRequest>();
    if (m_conf->Version.IsAtLeast(V3_8_0_0))
    {
        request->m_version = 5;
    }
    else if (m_conf->Version.IsAtLeast(V2_6_0_0))
    {
        request->m_version = 4;
    }
    else if (m_conf->Version.IsAtLeast(V2_4_0_0))
    {
        request->m_version = 3;
    }
    else if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 2;
    }
    else if (m_conf->Version.IsAtLeast(V0_11_0_0))
    {
        request->m_version = 1;
    }
    ResponsePromise<ListGroupsResponse> response;
    int err = co_await broker->ListGroups(request, response);
    if (err != 0)
    {
        err_chan.push_back(err);
        co_return err;
    }
    out.insert(response.m_response->m_groups.begin(), response.m_response->m_groups.end());
    co_return 0;
}
coev::awaitable<int> ClusterAdmin::_ListConsumerGroupOffsets(const std::string &group, std::shared_ptr<OffsetFetchRequest> request, std::shared_ptr<OffsetFetchResponse> &out)
{
    std::shared_ptr<Broker> coordinator;
    int err = co_await m_client->GetCoordinator(group, coordinator);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<OffsetFetchResponse> local_out;
    err = co_await coordinator->FetchOffset(request, local_out);
    if (err != 0)
    {
        if (isRetriableGroupCoordinatorError(err))
        {
            m_client->RefreshCoordinator(group);
        }
        co_return err;
    }
    if (local_out.m_response->m_err != ErrNoError)
    {
        if (isRetriableGroupCoordinatorError(local_out.m_response->m_err))
        {
            m_client->RefreshCoordinator(group);
        }
        co_return local_out.m_response->m_err;
    }
    out = local_out.m_response;
    co_return 0;
}
coev::awaitable<int> ClusterAdmin::ListConsumerGroupOffsets(const std::string &group,
                                                            const std::map<std::string, std::vector<int32_t>> &topicPartitions,
                                                            std::shared_ptr<OffsetFetchResponse> &out)
{
    auto request = std::make_shared<OffsetFetchRequest>(m_conf->Version, group, topicPartitions);
    co_return co_await RetryOnError(isRetriableGroupCoordinatorError, &ClusterAdmin::_ListConsumerGroupOffsets, group, request, out);
}

coev::awaitable<int> ClusterAdmin::_DeleteConsumerGroupOffset(std::shared_ptr<DeleteOffsetsRequest> request, const std::string &group, const std::string &topic, int32_t partition)
{
    std::shared_ptr<Broker> coordinator;
    int err = co_await m_client->GetCoordinator(group, coordinator);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<DeleteOffsetsResponse> response;
    err = co_await coordinator->DeleteOffsets(request, response);
    if (err != 0)
    {
        if (isRetriableGroupCoordinatorError(err))
        {
            m_client->RefreshCoordinator(group);
        }
        co_return err;
    }
    if (response.m_response->m_error_code != ErrNoError)
    {
        if (isRetriableGroupCoordinatorError(response.m_response->m_error_code))
        {
            m_client->RefreshCoordinator(group);
        }
        co_return response.m_response->m_error_code;
    }
    auto topicIt = response.m_response->m_errors.find(topic);
    if (topicIt != response.m_response->m_errors.end())
    {
        auto partIt = topicIt->second.find(partition);
        if (partIt != topicIt->second.end() && partIt->second != ErrNoError)
        {
            co_return partIt->second;
        }
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DeleteConsumerGroupOffset(const std::string &group, const std::string &topic, int32_t partition)
{
    auto request = std::make_shared<DeleteOffsetsRequest>();
    request->m_group = group;
    request->m_partitions[topic] = {partition};

    co_return co_await RetryOnError(isRetriableGroupCoordinatorError, &ClusterAdmin::_DeleteConsumerGroupOffset, request, group, topic, partition);
}

coev::awaitable<int> ClusterAdmin::_DeleteConsumerGroup(std::shared_ptr<DeleteGroupsRequest> request, const std::string &group)
{
    std::shared_ptr<Broker> coordinator;
    int err = co_await m_client->GetCoordinator(group, coordinator);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<DeleteGroupsResponse> response;
    err = co_await coordinator->DeleteGroups(request, response);
    if (err != 0)
    {
        if (isRetriableGroupCoordinatorError(err))
        {
            m_client->RefreshCoordinator(group);
        }
        co_return err;
    }
    auto it = response.m_response->m_group_error_codes.find(group);
    if (it == response.m_response->m_group_error_codes.end())
    {
        co_return ErrIncompleteResponse;
    }
    if (it->second != ErrNoError)
    {
        if (isRetriableGroupCoordinatorError(it->second))
        {
            err = co_await m_client->RefreshCoordinator(group);
            if (err != 0)
            {
                co_return err;
            }
        }
        co_return it->second;
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DeleteConsumerGroup(const std::string &group)
{
    auto request = std::make_shared<DeleteGroupsRequest>();
    request->m_groups = {group};
    if (m_conf->Version.IsAtLeast(V2_4_0_0))
    {
        request->m_version = 2;
    }
    else if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    co_return co_await RetryOnError(isRetriableGroupCoordinatorError, &ClusterAdmin::_DeleteConsumerGroup, request, group);
}

coev::awaitable<int> ClusterAdmin::DescribeLogDirs(const std::vector<int32_t> &broker_ids,
                                                   std::map<int32_t, std::vector<DescribeLogDirsResponseDirMetadata>> &out)
{
    struct result
    {
        int32_t id;
        std::vector<DescribeLogDirsResponseDirMetadata> logdirs;
        result(int32_t i, std::vector<DescribeLogDirsResponseDirMetadata> ld) : id(i), logdirs(std::move(ld)) {}
    };
    std::list<result> log_dirs_results;
    std::list<int> err_chan;

    co_task task;
    for (int32_t bid : broker_ids)
    {
        std::shared_ptr<Broker> broker;
        int err = FindBroker(bid, broker);
        if (err != 0)
        {
            LOG_CORE("Admin::DescribeClusterResponse Unable to find broker with ID = %d", bid);
            continue;
        }
        task << [this](auto broker, auto &log_dirs_results, auto &err_chan) -> coev::awaitable<void>
        {
            auto err = co_await broker->Open(m_conf);
            if (err != 0)
            {
                err_chan.push_back(err);
                co_return;
            }
            auto request = std::make_shared<DescribeLogDirsRequest>();
            if (m_conf->Version.IsAtLeast(V3_3_0_0))
            {
                request->m_version = 4;
            }
            else if (m_conf->Version.IsAtLeast(V3_2_0_0))
            {
                request->m_version = 3;
            }
            else if (m_conf->Version.IsAtLeast(V2_6_0_0))
            {
                request->m_version = 2;
            }
            else if (m_conf->Version.IsAtLeast(V2_0_0_0))
            {
                request->m_version = 1;
            }

            ResponsePromise<DescribeLogDirsResponse> response;
            err = co_await broker->DescribeLogDirs(request, response);
            if (err != 0)
            {
                err_chan.push_back(err);
                co_return;
            }
            if (response.m_response->m_error_code != ErrNoError)
            {
                err_chan.push_back(response.m_response->m_error_code);
                co_return;
            }
            log_dirs_results.emplace_back(broker->ID(), std::move(response.m_response->m_log_dirs));
        }(broker, log_dirs_results, err_chan);
    }
    co_await task.wait_all();

    for (auto &it : log_dirs_results)
    {
        out[it.id] = std::move(it.logdirs);
    }
    if (err_chan.size() > 0)
    {
        co_return err_chan.front();
    }
    co_return ErrNoError;
}

coev::awaitable<int> ClusterAdmin::DescribeUserScramCredentials(const std::vector<std::string> &users, std::vector<DescribeUserScramCredentialsResult> &out_results)
{
    auto req = std::make_shared<DescribeUserScramCredentialsRequest>();
    for (auto &u : users)
    {
        req->m_describe_users.push_back(DescribeUserScramCredentialsRequestUser{u});
    }

    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<DescribeUserScramCredentialsResponse> rsp;
    err = co_await broker->DescribeUserScramCredentials(req, rsp);
    if (err != 0)
    {
        co_return err;
    }
    out_results = std::move(rsp.m_response->m_results);
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::UpsertUserScramCredentials(const std::vector<AlterUserScramCredentialsUpsert> &upsert_ops, std::vector<AlterUserScramCredentialsResult> &out)
{
    return AlterUserScramCredentials(upsert_ops, {}, out);
}

coev::awaitable<int> ClusterAdmin::DeleteUserScramCredentials(const std::vector<AlterUserScramCredentialsDelete> &delete_ops, std::vector<AlterUserScramCredentialsResult> &out)
{
    return AlterUserScramCredentials({}, delete_ops, out);
}

coev::awaitable<int> ClusterAdmin::_AlterUserScramCredentials(std::shared_ptr<AlterUserScramCredentialsRequest> req, ResponsePromise<AlterUserScramCredentialsResponse> &rsp)
{
    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    err = co_await broker->AlterUserScramCredentials(req, rsp);
    co_return err;
}

coev::awaitable<int> ClusterAdmin::AlterUserScramCredentials(const std::vector<AlterUserScramCredentialsUpsert> &u, const std::vector<AlterUserScramCredentialsDelete> &d, std::vector<AlterUserScramCredentialsResult> &out_results)
{
    auto req = std::make_shared<AlterUserScramCredentialsRequest>();
    req->m_deletions = d;
    req->m_upsertions = u;

    ResponsePromise<AlterUserScramCredentialsResponse> rsp;
    int err = co_await RetryOnError(isRetriableControllerError, &ClusterAdmin::_AlterUserScramCredentials, req, rsp);

    if (err != 0)
    {
        out_results.clear();
        co_return err;
    }

    out_results = std::move(rsp.m_response->m_results);
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DescribeClientQuotas(const std::vector<QuotaFilterComponent> &components, bool strict, std::vector<DescribeClientQuotasEntry> &out)
{
    auto request = std::make_shared<DescribeClientQuotasRequest>(m_conf->Version, components, strict);
    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<DescribeClientQuotasResponse> rsp;
    err = co_await broker->DescribeClientQuotas(request, rsp);
    if (err != 0)
    {
        co_return err;
    }
    if (!rsp.m_response->m_error_msg.empty())
    {
        co_return ErrDescribeClientQuotas;
    }
    if (rsp.m_response->m_error_code != ErrNoError)
    {
        co_return rsp.m_response->m_error_code;
    }
    out = std::move(rsp.m_response->m_entries);
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::AlterClientQuotas(const std::vector<QuotaEntityComponent> &entity, const ClientQuotasOp &op, bool validate_only)
{
    AlterClientQuotasEntry entry;
    entry.m_entity = entity;
    entry.m_ops = {op};

    auto request = std::make_shared<AlterClientQuotasRequest>();
    request->m_entries = {entry};
    request->m_validate_only = validate_only;

    std::shared_ptr<Broker> broker;
    int err = co_await GetController(broker);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<AlterClientQuotasResponse> rsp;
    err = co_await broker->AlterClientQuotas(request, rsp);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &entry : rsp.m_response->m_entries)
    {
        if (!entry.m_error_msg.empty())
        {
            co_return ErrAlterClientQuotas;
        }
        if (entry.m_error_code != ErrNoError)
        {
            co_return entry.m_error_code;
        }
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::_RemoveMemberFromConsumerGroup(std::shared_ptr<LeaveGroupRequest> request, std::shared_ptr<LeaveGroupResponse> &response, const std::string &groupId)
{
    std::shared_ptr<Broker> coordinator;
    int err = co_await m_client->GetCoordinator(groupId, coordinator);
    if (err != 0)
    {
        co_return err;
    }
    ResponsePromise<LeaveGroupResponse> out;
    err = co_await coordinator->LeaveGroup(request, out);
    if (err != 0)
    {
        if (isRetriableGroupCoordinatorError(err))
        {
            m_client->RefreshCoordinator(groupId);
        }
        co_return err;
    }
    if (out.m_response->m_err != ErrNoError)
    {
        if (isRetriableGroupCoordinatorError(out.m_response->m_err))
        {
            m_client->RefreshCoordinator(groupId);
        }
        co_return out.m_response->m_err;
    }
    response = out.m_response;
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::RemoveMemberFromConsumerGroup(const std::string &groupId, const std::vector<std::string> &groupInstanceIds, std::shared_ptr<LeaveGroupResponse> &response)
{
    if (!m_conf->Version.IsAtLeast(V2_4_0_0))
    {
        co_return ErrUnsupportedVersion;
    }

    auto request = std::make_shared<LeaveGroupRequest>();
    request->m_version = 3;
    request->m_group_id = groupId;
    for (auto &instanceId : groupInstanceIds)
    {
        MemberIdentity member;
        member.m_group_instance_id = instanceId;
        request->m_members.push_back(member);
    }

    co_return co_await RetryOnError(isRetriableGroupCoordinatorError, &ClusterAdmin::_RemoveMemberFromConsumerGroup, request, response, groupId);
}