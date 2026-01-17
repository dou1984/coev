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

coev::awaitable<int> ClusterAdmin::RetryOnError(std::function<bool(int)> retryable, std::function<coev::awaitable<int>()> f)
{
    int attemptsRemaining = m_conf->Admin.Retry.Max + 1;
    while (true)
    {
        int err = co_await f();
        attemptsRemaining--;
        if (err == 0 || attemptsRemaining <= 0 || !retryable(err))
        {
            co_return err;
        }
        LOG_CORE("Admin::request retrying after %dms... (%d attempts remaining)",
                       static_cast<int>(m_conf->Admin.Retry.Backoff.count()), attemptsRemaining);
        co_await sleep_for(m_conf->Admin.Retry.Backoff);
    }
}

int ClusterAdmin::FindBroker(int32_t id, std::shared_ptr<Broker> &out_broker)
{
    auto brokers = m_client->Brokers();
    for (auto &b : brokers)
    {
        if (b->ID() == id)
        {
            out_broker = b;
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

coev::awaitable<int> ClusterAdmin::CreateTopic(const std::string &topic, const std::shared_ptr<TopicDetail> &detail, bool validateOnly)
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
    auto request = NewCreateTopicsRequest(m_conf->Version, topicDetails, m_conf->Admin.Timeout.count(), validateOnly);
    co_return co_await RetryOnError(
        isRetriableControllerError,
        [this, &request, &topic]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> b;
            int err = co_await GetController(b);
            if (err != 0)
            {
                co_return err;
            }
            CreateTopicsResponse rsp;
            err = co_await b->CreateTopics(*request, rsp);
            if (err != 0)
            {
                co_return err;
            }
            auto it = rsp.m_topic_errors.find(topic);
            if (it == rsp.m_topic_errors.end())
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
        });
}

coev::awaitable<int> ClusterAdmin::DescribeTopics(const std::vector<std::string> &topics,
                                                  std::vector<std::shared_ptr<TopicMetadata>> &out_metadata)
{
    int err = co_await RetryOnError(
        isRetriableControllerError,
        [this, &topics, &out_metadata]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> controller;
            int err = co_await GetController(controller);
            if (err != 0)
            {
                co_return err;
            }
            auto request = NewMetadataRequest(m_conf->Version, topics);
            MetadataResponse response;
            err = co_await controller->GetMetadata(*request, response);
            if (isRetriableControllerError(err))
            {
                std::shared_ptr<Broker> dummy;
                co_await RefreshController(dummy);
            }
            if (err != 0)
            {
                co_return err;
            }
            out_metadata = std::move(response.Topics);
            co_return 0;
        });
    co_return err;
}

coev::awaitable<int> ClusterAdmin::DescribeCluster(std::vector<std::shared_ptr<Broker>> &out_brokers, int32_t &out_controllerID)
{
    int err = co_await RetryOnError(
        isRetriableControllerError,
        [this, &out_brokers, &out_controllerID]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> controller;
            int err = co_await GetController(controller);
            if (err != 0)
            {
                co_return err;
            }
            auto request = NewMetadataRequest(m_conf->Version, std::vector<std::string>{});
            MetadataResponse response;
            err = co_await controller->GetMetadata(*request, response);
            if (isRetriableControllerError(err))
            {
                std::shared_ptr<Broker> dummy;
                RefreshController(dummy);
            }
            if (err != 0)
            {
                co_return err;
            }
            out_brokers = std::move(response.Brokers);
            out_controllerID = response.ControllerID;
            co_return 0;
        });
    if (err != 0)
    {
        out_brokers.clear();
        out_controllerID = 0;
    }
    co_return err;
}

coev::awaitable<int> ClusterAdmin::ListTopics(std::map<std::string, TopicDetail> &topicsDetailsMap)
{
    std::shared_ptr<Broker> b;
    int err = FindAnyBroker(b);
    if (err != 0)
    {
        co_return err;
    }
    err = co_await b->Open(m_conf);
    auto metadataReq = NewMetadataRequest(m_conf->Version, std::vector<std::string>{});
    MetadataResponse metadataResp;
    err = co_await b->GetMetadata(*metadataReq, metadataResp);
    if (err != 0)
    {
        co_return err;
    }

    std::vector<std::shared_ptr<ConfigResource>> describeConfigsResources;
    for (auto &topic : metadataResp.Topics)
    {
        TopicDetail topicDetails;
        topicDetails.m_num_partitions = static_cast<int32_t>(topic->Partitions.size());
        if (!topic->Partitions.empty())
        {
            topicDetails.m_replica_assignment.clear();
            for (auto &partition : topic->Partitions)
            {
                topicDetails.m_replica_assignment[partition->m_id] = partition->m_replicas;
            }
            topicDetails.m_replication_factor = static_cast<int16_t>(topic->Partitions[0]->m_replicas.size());
        }
        topicsDetailsMap[topic->Name] = topicDetails;
        auto topicResource = std::make_shared<ConfigResource>();
        topicResource->m_type = TopicResource;
        topicResource->m_name = topic->Name;
        describeConfigsResources.push_back(topicResource);
    }

    auto describeConfigsReq = std::make_shared<DescribeConfigsRequest>();
    describeConfigsReq->m_resources = {describeConfigsResources};
    if (m_conf->Version.IsAtLeast(V1_1_0_0))
    {
        describeConfigsReq->m_version = 1;
    }
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        describeConfigsReq->m_version = 2;
    }

    DescribeConfigsResponse describeConfigsResp;
    err = co_await b->DescribeConfigs(*describeConfigsReq, describeConfigsResp);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &resource : describeConfigsResp.m_resources)
    {
        auto it = topicsDetailsMap.find(resource->m_name);
        if (it == topicsDetailsMap.end())
            continue;
        TopicDetail &topicDetails = it->second;
        topicDetails.m_config_entries.clear();
        for (auto &entry : resource->m_configs)
        {
            if (entry->m_default || entry->m_sensitive)
                continue;
            topicDetails.m_config_entries[entry->m_name] = entry->m_value;
        }
        topicsDetailsMap[resource->m_name] = topicDetails;
    }

    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DeleteTopic(const std::string &topic)
{
    if (topic.empty())
    {
        co_return ErrInvalidTopic;
    }
    auto request = std::make_shared<DeleteTopicsRequest>();
    request->m_topics = {topic};
    request->m_timeout = m_conf->Admin.Timeout;
    if (m_conf->Version.IsAtLeast(V2_4_0_0))
    {
        request->m_version = 4;
    }
    else if (m_conf->Version.IsAtLeast(V2_1_0_0))
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

    co_return co_await RetryOnError(
        isRetriableControllerError,
        [this, &topic, &request]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> b;
            int err = co_await GetController(b);
            if (err != 0)
            {
                co_return err;
            }
            DeleteTopicsResponse rsp;
            err = co_await b->DeleteTopics(*request, rsp);
            if (err != 0)
            {
                co_return err;
            }
            auto it = rsp.m_topic_error_codes.find(topic);
            if (it == rsp.m_topic_error_codes.end())
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
        });
}

coev::awaitable<int> ClusterAdmin::CreatePartitions(const std::string &topic, int32_t count, const std::vector<std::vector<int32_t>> &assignment, bool validateOnly)
{
    if (topic.empty())
    {
        co_return ErrInvalidTopic;
    }
    std::map<std::string, std::shared_ptr<TopicPartition>> topicPartitions;
    auto tp = std::make_shared<TopicPartition>();
    tp->m_count = count;
    tp->m_assignment = assignment;
    topicPartitions[topic] = tp;

    auto request = std::make_shared<CreatePartitionsRequest>();
    request->m_topic_partitions = topicPartitions;
    request->m_timeout = m_conf->Admin.Timeout;
    request->m_validate_only = validateOnly;
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    co_return co_await RetryOnError(
        isRetriableControllerError,
        [this, &request, &topic]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> b;
            int err = co_await GetController(b);
            if (err != 0)
            {
                co_return err;
            }
            CreatePartitionsResponse rsp;
            err = co_await b->CreatePartitions(*request, rsp);
            if (err != 0)
            {
                co_return err;
            }
            auto it = rsp.m_topic_partition_errors.find(topic);
            if (it == rsp.m_topic_partition_errors.end())
            {
                co_return ErrIncompleteResponse;
            }
            if (it->second->m_err != ErrNoError)
            {
                if (it->second->m_err == ErrNotController)
                {
                    std::shared_ptr<Broker> dummy;
                    RefreshController(dummy);
                }
                co_return it->second->m_err;
            }
            co_return 0;
        });
}

coev::awaitable<int> ClusterAdmin::AlterPartitionReassignments(const std::string &topic,
                                                               const std::vector<std::vector<int32_t>> &assignment)
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
        request->AddBlock(topic, static_cast<int32_t>(i), assignment[i]);
    }

    co_return co_await RetryOnError(
        isRetriableControllerError,
        [this, &request]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> b;
            int err = co_await GetController(b);
            if (err != 0)
            {
                co_return err;
            }
            AlterPartitionReassignmentsResponse response;
            err = co_await b->AlterPartitionReassignments(*request, response);
            std::vector<int> errs;
            if (err != 0)
            {
                errs.push_back(err);
            }
            else
            {
                if (response.m_error_code > 0)
                {
                    errs.push_back(response.m_error_code);
                }
                for (auto &topicErrors : response.m_errors)
                {
                    for (auto &partitionError : topicErrors.second)
                    {
                        if (partitionError.second->m_error_code != ErrNoError)
                        {
                            errs.push_back(partitionError.second->m_error_code);
                        }
                    }
                }
            }
            if (!errs.empty())
            {

                co_return ErrReassignPartitions;
            }
            co_return 0;
        });
}

coev::awaitable<int> ClusterAdmin::ListPartitionReassignments(
    const std::string &topic,
    const std::vector<int32_t> &partitions,
    std::map<std::string, std::map<int32_t, std::shared_ptr<PartitionReplicaReassignmentsStatus>>> &out_topicStatus)
{
    if (topic.empty())
    {
        co_return ErrInvalidTopic;
    }
    auto request = std::make_shared<ListPartitionReassignmentsRequest>();
    request->m_timeout = std::chrono::milliseconds(60000);
    request->m_version = 0;
    request->AddBlock(topic, partitions);

    int err = co_await RetryOnError(
        isRetriableControllerError,
        [this, &request, &out_topicStatus]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> b;
            int err = co_await GetController(b);
            if (err != 0)
            {
                co_return err;
            }
            err = co_await b->Open(m_client->GetConfig());
            if (err != 0)
            {
                co_return err;
            }
            ListPartitionReassignmentsResponse response;
            err = co_await b->ListPartitionReassignments(*request, response);
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
                out_topicStatus = response.m_topic_status;
            }
            co_return err;
        });

    if (err != 0)
    {
        out_topicStatus.clear();
    }
    co_return err;
}

coev::awaitable<int> ClusterAdmin::DeleteRecords(const std::string &topic, const std::map<int32_t, int64_t> &partitionOffsets)
{
    if (topic.empty())
    {
        co_return ErrInvalidTopic;
    }
    std::vector<int> errs;
    std::map<std::shared_ptr<Broker>, std::vector<int32_t>> partitionPerBroker;
    for (auto &kv : partitionOffsets)
    {
        int32_t partition = kv.first;
        std::shared_ptr<Broker> broker;
        int err = co_await m_client->Leader(topic, partition, broker);
        if (err != 0)
        {
            errs.push_back(err);
            continue;
        }
        partitionPerBroker[broker].push_back(partition);
    }

    for (auto &kv : partitionPerBroker)
    {
        auto broker = kv.first;
        auto &partitions = kv.second;
        std::unordered_map<int32_t, int64_t> recordsToDelete;
        for (int32_t p : partitions)
        {
            recordsToDelete[p] = partitionOffsets.at(p);
        }
        std::unordered_map<std::string, std::shared_ptr<DeleteRecordsRequestTopic>> topics;
        auto topicData = std::make_shared<DeleteRecordsRequestTopic>();
        topicData->m_partition_offsets = std::move(recordsToDelete);
        topics[topic] = topicData;

        auto request = std::make_shared<DeleteRecordsRequest>();
        request->m_topics = std::move(topics);
        request->m_timeout = m_conf->Admin.Timeout;
        if (m_conf->Version.IsAtLeast(V2_0_0_0))
        {
            request->m_version = 1;
        }

        DeleteRecordsResponse rsp;
        int err = co_await broker->DeleteRecords(*request, rsp);
        if (err != 0)
        {
            errs.emplace_back(err);
            continue;
        }

        auto it = rsp.m_topics.find(topic);
        if (it == rsp.m_topics.end())
        {
            errs.emplace_back(ErrIncompleteResponse);
            continue;
        }

        for (auto &part : it->second->m_partitions)
        {
            if (part.second->m_err != ErrNoError)
            {
                errs.emplace_back(part.second->m_err);
            }
        }
    }

    if (!errs.empty())
    {
        co_return ErrDeleteRecords;
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DescribeConfig(const ConfigResource &resource, std::vector<std::shared_ptr<ConfigEntry>> &out_entries)
{
    std::vector<std::shared_ptr<ConfigResource>> resources;
    resources.push_back(std::make_shared<ConfigResource>(resource));

    auto request = std::make_shared<DescribeConfigsRequest>();
    request->m_resources = resources;
    if (m_conf->Version.IsAtLeast(V1_1_0_0))
    {
        request->m_version = 1;
    }
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 2;
    }

    std::shared_ptr<Broker> b;
    int err;
    if (dependsOnSpecificNode(resource))
    {

        int32_t id = static_cast<int32_t>(std::stol(resource.m_name));
        err = FindBroker(id, b);
        if (err != 0)
        {
            co_return err;
        }
    }
    else
    {
        err = FindAnyBroker(b);
    }
    if (err != 0)
    {
        co_return err;
    }
    err = co_await b->Open(m_client->GetConfig());
    if (err != 0)
    {
        co_return err;
    }

    DescribeConfigsResponse rsp;
    err = co_await b->DescribeConfigs(*request, rsp);
    if (err != 0)
    {
        co_return err;
    }

    out_entries.clear();
    for (auto &rspResource : rsp.m_resources)
    {
        if (rspResource->m_name == resource.m_name)
        {
            if (rspResource->m_error_code != 0)
            {
                co_return rspResource->m_error_code;
            }
            for (auto &cfgEntry : rspResource->m_configs)
            {
                out_entries.push_back(cfgEntry);
            }
        }
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::AlterConfig(ConfigResourceType resourceType, const std::string &name,
                                               const std::map<std::string, std::string> &entries, bool validateOnly)
{
    std::vector<std::shared_ptr<AlterConfigsResource>> resources;
    auto res = std::make_shared<AlterConfigsResource>();
    res->m_type = resourceType;
    res->m_name = name;
    res->m_config_entries = entries;
    resources.push_back(res);

    auto request = std::make_shared<AlterConfigsRequest>();
    request->m_resources = resources;
    request->m_validate_only = validateOnly;
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    std::shared_ptr<Broker> b;
    int err;
    ConfigResource tmp = {
        .m_type = resourceType,
        .m_name = name,
    };
    if (dependsOnSpecificNode(tmp))
    {

        int32_t id = static_cast<int32_t>(std::stol(name));
        err = FindBroker(id, b);
        if (err != 0)
        {
            co_return err;
        }
    }
    else
    {
        err = FindAnyBroker(b);
        if (err != 0)
        {
            co_return err;
        }
    }

    err = co_await b->Open(m_client->GetConfig());
    if (err != 0)
    {
        co_return err;
    }

    AlterConfigsResponse rsp;
    err = co_await b->AlterConfigs(*request, rsp);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &rspResource : rsp.m_resources)
    {
        if (rspResource->m_name == name)
        {
            if (rspResource->m_error_code != 0)
            {
                co_return rspResource->m_error_code;
            }
        }
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::IncrementalAlterConfig(ConfigResourceType resourceType, const std::string &name, const std::map<std::string, IncrementalAlterConfigsEntry> &entries, bool validateOnly)
{
    std::vector<std::shared_ptr<IncrementalAlterConfigsResource>> resources;
    auto res = std::make_shared<IncrementalAlterConfigsResource>();
    res->m_type = resourceType;
    res->m_name = name;
    res->m_config_entries = entries;
    resources.push_back(res);

    auto request = std::make_shared<IncrementalAlterConfigsRequest>();
    request->m_resources = resources;
    request->m_validate_only = validateOnly;
    if (m_conf->Version.IsAtLeast(V2_4_0_0))
    {
        request->m_version = 1;
    }

    std::shared_ptr<Broker> b;
    int err;
    ConfigResource tmp = {
        .m_type = resourceType,
        .m_name = name,
    };
    if (dependsOnSpecificNode(tmp))
    {
        int32_t id = static_cast<int32_t>(std::stol(name));
        err = FindBroker(id, b);
        if (err != 0)
        {
            co_return err;
        }
    }
    else
    {
        err = FindAnyBroker(b);
    }
    if (err != 0)
    {
        co_return err;
    }
    err = co_await b->Open(m_client->GetConfig());

    IncrementalAlterConfigsResponse response;
    err = co_await b->IncrementalAlterConfigs(*request, response);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &rspResource : response.m_resources)
    {
        if (rspResource->m_name == name)
        {
            if (rspResource->m_error_code != ErrNoError)
            {
                int err = rspResource->m_error_code;
                if (!rspResource->m_error_msg.empty())
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
    std::vector<std::shared_ptr<AclCreation>> acls;
    acls.push_back(std::make_shared<AclCreation>(resource, acl));

    auto request = std::make_shared<CreateAclsRequest>();
    request->m_acl_creations = acls;
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    CreateAclsResponse rsp;
    co_return co_await b->CreateAcls(*request, rsp);
}

coev::awaitable<int> ClusterAdmin::CreateACLs(const std::vector<std::shared_ptr<ResourceAcls>> &resourceACLs)
{
    std::vector<std::shared_ptr<AclCreation>> acls;
    for (auto &resourceACL : resourceACLs)
    {
        for (auto &acl : resourceACL->m_acls)
        {
            acls.emplace_back(std::make_shared<AclCreation>(resourceACL->m_resource, *acl));
        }
    }

    auto request = std::make_shared<CreateAclsRequest>();
    request->m_acl_creations = std::move(acls);
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    CreateAclsResponse response;
    co_return co_await b->CreateAcls(*request, response);
}

coev::awaitable<int> ClusterAdmin::ListAcls(const AclFilter &filter, std::vector<std::shared_ptr<ResourceAcls>> &out_results)
{
    auto request = std::make_shared<DescribeAclsRequest>();
    request->m_filter = filter;
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    DescribeAclsResponse response;
    err = co_await b->DescribeAcls(*request, response);
    if (err != 0)
    {
        co_return err;
    }

    out_results.clear();
    for (auto &rAcl : response.m_resource_acls)
    {
        out_results.push_back(rAcl);
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DeleteACL(const AclFilter &filter, bool validateOnly, std::vector<std::shared_ptr<MatchingAcl>> &out_matchingAcls)
{
    std::vector<std::shared_ptr<AclFilter>> filters;
    filters.push_back(std::make_shared<AclFilter>(filter));

    auto request = std::make_shared<DeleteAclsRequest>();
    request->m_filters = filters;
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    DeleteAclsResponse rsp;
    err = co_await b->DeleteAcls(*request, rsp);
    if (err != 0)
    {
        co_return err;
    }

    out_matchingAcls.clear();
    for (auto &fr : rsp.m_filter_responses)
    {
        for (auto &mACL : fr->m_matching_acls)
        {
            out_matchingAcls.emplace_back(mACL);
        }
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::ElectLeaders(ElectionType electionType, const std::unordered_map<std::string, std::vector<int32_t>> &partitions,
                                                std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<PartitionResult>>> &out_results)
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

    ElectLeadersResponse res;
    int err = co_await RetryOnError(
        isRetriableControllerError,
        [this, &request, &res]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> b;
            int err = co_await GetController(b);
            if (err != 0)
            {
                co_return err;
            }
            err = co_await b->Open(m_client->GetConfig());
            if (err != 0)
            {
                co_return err;
            }
            err = co_await b->ElectLeaders(*request, res);
            if (err != 0)
            {
                co_return err;
            }
            if (res.m_error_code != ErrNoError)
            {
                if (isRetriableControllerError(res.m_error_code))
                {
                    std::shared_ptr<Broker> dummy;
                    RefreshController(dummy);
                }
                co_return res.m_error_code;
            }
            co_return 0;
        });

    if (err != 0)
    {
        out_results.clear();
        co_return err;
    }
    out_results = res.m_replica_election_results;
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DescribeConsumerGroups(const std::vector<std::string> &groups,
                                                          std::vector<std::shared_ptr<GroupDescription>> &out_result)
{
    std::map<std::shared_ptr<Broker>, std::vector<std::string>> groupsPerBroker;
    for (auto &group : groups)
    {
        std::shared_ptr<Broker> coordinator;
        int err = co_await m_client->GetCoordinator(group, coordinator);
        if (err != 0)
        {
            co_return err;
        }
        groupsPerBroker[coordinator].push_back(group);
    }

    out_result.clear();
    for (auto &kv : groupsPerBroker)
    {
        auto broker = kv.first;
        auto &brokerGroups = kv.second;
        auto describeReq = std::make_shared<DescribeGroupsRequest>();
        describeReq->m_groups = brokerGroups;
        if (m_conf->Version.IsAtLeast(V2_4_0_0))
        {
            describeReq->m_version = 5;
        }
        else if (m_conf->Version.IsAtLeast(V2_3_0_0))
        {
            describeReq->m_version = 3;
        }
        else if (m_conf->Version.IsAtLeast(V2_0_0_0))
        {
            describeReq->m_version = 2;
        }
        else if (m_conf->Version.IsAtLeast(V1_1_0_0))
        {
            describeReq->m_version = 1;
        }

        DescribeGroupsResponse response;
        int err = co_await broker->DescribeGroups(*describeReq, response);
        if (err != 0)
        {
            co_return err;
        }
        out_result.insert(out_result.end(), response.m_groups.begin(), response.m_groups.end());
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::ListConsumerGroups(std::map<std::string, std::string> &out_groups)
{
    auto brokers = m_client->Brokers();

    std::list<int> errChan;
    coev::co_task _task;
    for (auto &b : brokers)
    {
        _task << [this, b, &out_groups, &errChan]() -> coev::awaitable<int>
        {
            b->Open(m_conf);
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
            ListGroupsResponse response;
            int err = co_await b->ListGroups(*request, response);
            if (err != 0)
            {
                errChan.push_back(err);
                co_return err;
            }
            out_groups.insert(response.m_groups.begin(), response.m_groups.end());
            co_return 0;
        }();
    }

    co_await _task.wait_all();

    co_return 0;
}

coev::awaitable<int> ClusterAdmin::ListConsumerGroupOffsets(const std::string &group,
                                                            const std::map<std::string, std::vector<int32_t>> &topicPartitions,
                                                            std::shared_ptr<OffsetFetchResponse> &out)
{
    auto request = OffsetFetchRequest::NewOffsetFetchRequest(m_conf->Version, group, topicPartitions);
    co_return co_await RetryOnError(
        isRetriableGroupCoordinatorError,
        [this, &request, &out, &group]() -> coev::awaitable<int>
        {
        std::shared_ptr<Broker> coordinator;
        int err =co_await m_client->GetCoordinator(group, coordinator);
        if (err != 0) {
            co_return err;
        }
        OffsetFetchResponse local_out;
        err =co_await coordinator->FetchOffset(*request, local_out);
        if (err != 0) {
            if (isRetriableGroupCoordinatorError(err)) 
            {
                m_client->RefreshCoordinator(group);
            }
            co_return err;
        }
        if (local_out.m_err != ErrNoError)
         {
            if (isRetriableGroupCoordinatorError(local_out.m_err))
             {
                m_client->RefreshCoordinator(group);
            }
            co_return local_out.m_err;
        }
        // Convert local_out to shared_ptr
        out = std::make_shared<OffsetFetchResponse>(local_out);
        co_return 0; });
}

coev::awaitable<int> ClusterAdmin::DeleteConsumerGroupOffset(const std::string &group, const std::string &topic, int32_t partition)
{
    auto request = std::make_shared<DeleteOffsetsRequest>();
    request->m_group = group;
    request->m_partitions[topic] = {partition};

    co_return co_await RetryOnError(
        isRetriableGroupCoordinatorError,
        [this, &request, &group, &topic, &partition]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> coordinator;
            int err = co_await m_client->GetCoordinator(group, coordinator);
            if (err != 0)
            {
                co_return err;
            }
            DeleteOffsetsResponse response;
            err = co_await coordinator->DeleteOffsets(*request, response);
            if (err != 0)
            {
                if (isRetriableGroupCoordinatorError(err))
                {
                    m_client->RefreshCoordinator(group);
                }
                co_return err;
            }
            if (response.m_error_code != ErrNoError)
            {
                if (isRetriableGroupCoordinatorError(response.m_error_code))
                {
                    m_client->RefreshCoordinator(group);
                }
                co_return response.m_error_code;
            }
            auto topicIt = response.m_errors.find(topic);
            if (topicIt != response.m_errors.end())
            {
                auto partIt = topicIt->second.find(partition);
                if (partIt != topicIt->second.end() && partIt->second != ErrNoError)
                {
                    co_return partIt->second;
                }
            }
            co_return 0;
        });
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

    co_return co_await RetryOnError(
        isRetriableGroupCoordinatorError,
        [this, &request, &group]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> coordinator;
            int err = co_await m_client->GetCoordinator(group, coordinator);
            if (err != 0)
            {
                co_return err;
            }
            DeleteGroupsResponse response;
            err = co_await coordinator->DeleteGroups(*request, response);
            if (err != 0)
            {
                if (isRetriableGroupCoordinatorError(err))
                {
                    m_client->RefreshCoordinator(group);
                }
                co_return err;
            }
            auto it = response.m_group_error_codes.find(group);
            if (it == response.m_group_error_codes.end())
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
        });
}

coev::awaitable<int> ClusterAdmin::DescribeLogDirs(const std::vector<int32_t> &brokerIds,
                                                   std::map<int32_t, std::vector<DescribeLogDirsResponseDirMetadata>> &out)
{
    struct result
    {
        int32_t id;
        std::vector<DescribeLogDirsResponseDirMetadata> logdirs;
    };
    std::list<result> logDirsResults;
    std::list<int> errChan;

    co_task task_;
    for (int32_t b_id : brokerIds)
    {
        std::shared_ptr<Broker> broker;
        int err = FindBroker(b_id, broker);
        if (err != 0)
        {
            LOG_CORE("Admin::DescribeClusterResponse Unable to find broker with ID = %d", b_id);
            continue;
        }
        task_ << [this](std::shared_ptr<Broker> broker, std::list<result> &logDirsResults, std::list<int> &errChan) -> coev::awaitable<void>
        {
            auto err = co_await broker->Open(m_conf);
            if (err != 0)
            {
                errChan.push_back(err);
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

            DescribeLogDirsResponse response;
            err = co_await broker->DescribeLogDirs(*request, response);
            if (err != 0)
            {
                errChan.push_back(err);
                co_return;
            }
            if (response.m_error_code != ErrNoError)
            {
                errChan.push_back(response.m_error_code);
                co_return;
            }
            logDirsResults.emplace_back(broker->ID(), std::move(response.m_log_dirs));
        }(broker, logDirsResults, errChan);
    }
    co_await task_.wait_all();

    for (auto &it : logDirsResults)
    {
        out[it.id] = std::move(it.logdirs);
    }
    if (errChan.size() > 0)
    {
        co_return errChan.front();
    }
    co_return ErrNoError;
}

coev::awaitable<int> ClusterAdmin::DescribeUserScramCredentials(const std::vector<std::string> &users, std::vector<std::shared_ptr<DescribeUserScramCredentialsResult>> &out_results)
{
    auto req = std::make_shared<DescribeUserScramCredentialsRequest>();
    for (auto &u : users)
    {
        req->m_describe_users.push_back(DescribeUserScramCredentialsRequestUser{u});
    }

    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    DescribeUserScramCredentialsResponse rsp;
    err = co_await b->DescribeUserScramCredentials(*req, rsp);
    if (err != 0)
    {
        co_return err;
    }
    out_results = std::move(rsp.m_results);
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::UpsertUserScramCredentials(const std::vector<AlterUserScramCredentialsUpsert> &upsert_ops, std::vector<std::shared_ptr<AlterUserScramCredentialsResult>> &out_results)
{
    return AlterUserScramCredentials(upsert_ops, {}, out_results);
}

coev::awaitable<int> ClusterAdmin::DeleteUserScramCredentials(const std::vector<AlterUserScramCredentialsDelete> &delete_ops, std::vector<std::shared_ptr<AlterUserScramCredentialsResult>> &out_results)
{
    return AlterUserScramCredentials({}, delete_ops, out_results);
}

coev::awaitable<int> ClusterAdmin::AlterUserScramCredentials(const std::vector<AlterUserScramCredentialsUpsert> &u, const std::vector<AlterUserScramCredentialsDelete> &d, std::vector<std::shared_ptr<AlterUserScramCredentialsResult>> &out_results)
{
    auto req = std::make_shared<AlterUserScramCredentialsRequest>();
    req->m_deletions = d;
    req->m_upsertions = u;

    AlterUserScramCredentialsResponse rsp;
    int err = co_await RetryOnError(
        isRetriableControllerError,
        [this, &req, &rsp]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> b;
            int err = co_await GetController(b);
            if (err != 0)
            {
                co_return err;
            }
            err = co_await b->AlterUserScramCredentials(*req, rsp);
            co_return err;
        });

    if (err != 0)
    {
        out_results.clear();
        co_return err;
    }
    out_results = std::move(rsp.m_results);
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DescribeClientQuotas(const std::vector<QuotaFilterComponent> &components, bool strict, std::vector<DescribeClientQuotasEntry> &out)
{
    auto request = NewDescribeClientQuotasRequest(m_conf->Version, components, strict);
    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    DescribeClientQuotasResponse rsp;
    err = co_await b->DescribeClientQuotas(*request, rsp);
    if (err != 0)
    {
        co_return err;
    }
    if (!rsp.m_error_msg.empty())
    {
        co_return ErrDescribeClientQuotas;
    }
    if (rsp.m_error_code != ErrNoError)
    {
        co_return rsp.m_error_code;
    }
    out = std::move(rsp.m_entries);
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::AlterClientQuotas(const std::vector<QuotaEntityComponent> &entity, const ClientQuotasOp &op, bool validateOnly)
{
    AlterClientQuotasEntry entry;
    entry.m_entity = entity;
    entry.m_ops = {op};

    auto request = std::make_shared<AlterClientQuotasRequest>();
    request->m_entries = {entry};
    request->m_validate_only = validateOnly;

    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    AlterClientQuotasResponse rsp;
    err = co_await b->AlterClientQuotas(*request, rsp);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &entry : rsp.m_entries)
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

coev::awaitable<int> ClusterAdmin::RemoveMemberFromConsumerGroup(const std::string &groupId, const std::vector<std::string> &groupInstanceIds, std::shared_ptr<LeaveGroupResponse> &out_response)
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

    co_return co_await RetryOnError(
        isRetriableGroupCoordinatorError,
        [this, &request, &groupId, &out_response]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> coordinator;
            int err = co_await m_client->GetCoordinator(groupId, coordinator);
            if (err != 0)
            {
                co_return err;
            }
            LeaveGroupResponse local_out;
            err = co_await coordinator->LeaveGroup(*request, local_out);
            if (err != 0)
            {
                if (isRetriableGroupCoordinatorError(err))
                {
                    m_client->RefreshCoordinator(groupId);
                }
                co_return err;
            }
            if (local_out.m_err != ErrNoError)
            {
                if (isRetriableGroupCoordinatorError(local_out.m_err))
                {
                    m_client->RefreshCoordinator(groupId);
                }
                co_return local_out.m_err;
            }
            // Convert local_out to shared_ptr
            out_response = std::make_shared<LeaveGroupResponse>(local_out);
            co_return 0;
        });
}