#include <random>
#include <thread>
#include <chrono>

#include "admin.h"
#include "errors.h"
#include "logger.h"
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
#include "metrics.h"

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
    return (resource.Type == BrokerResource && !resource.Name.empty()) ||
           resource.Type == BrokerLoggerResource;
}

std::shared_ptr<ClusterAdmin> ClusterAdmin::Create(std::shared_ptr<Client> client, std::shared_ptr<Config> conf)
{
    return std::make_shared<ClusterAdmin>(client, conf);
}

ClusterAdmin::ClusterAdmin(std::shared_ptr<Client> client, std::shared_ptr<Config> conf)
    : client_(client), conf_(conf) {}

int ClusterAdmin::Close()
{
    return client_->Close();
}

coev::awaitable<int> ClusterAdmin::GetController(std::shared_ptr<Broker> &out)
{
    return client_->Controller(out);
}

coev::awaitable<int> ClusterAdmin::GetCoordinator(const std::string &group, std::shared_ptr<Broker> &out)
{
    return client_->GetCoordinator(group, out);
}

coev::awaitable<int> ClusterAdmin::RefreshController(std::shared_ptr<Broker> &out)
{
    return client_->RefreshController(out);
}

coev::awaitable<int> ClusterAdmin::RetryOnError(std::function<bool(int)> retryable, std::function<coev::awaitable<int>()> f)
{
    int attemptsRemaining = conf_->Admin.Retry.Max + 1;
    while (true)
    {
        int err = co_await f();
        attemptsRemaining--;
        if (err == 0 || attemptsRemaining <= 0 || !retryable(err))
        {
            co_return err;
        }
        Logger::Printf("admin/request retrying after %dms... (%d attempts remaining)\n",
                       static_cast<int>(conf_->Admin.Retry.Backoff.count()), attemptsRemaining);
        co_await sleep_for(conf_->Admin.Retry.Backoff);
    }
}

int ClusterAdmin::FindBroker(int32_t id, std::shared_ptr<Broker> &out_broker)
{
    auto brokers = client_->Brokers();
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
    auto brokers = client_->Brokers();
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
    auto request = NewCreateTopicsRequest(conf_->Version, topicDetails, conf_->Admin.Timeout.count(), validateOnly);
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
            std::shared_ptr<CreateTopicsResponse> rsp;
            err = co_await b->CreateTopics(request, rsp);
            if (err != 0)
            {
                co_return err;
            }
            auto it = rsp->TopicErrors.find(topic);
            if (it == rsp->TopicErrors.end())
            {
                co_return ErrIncompleteResponse;
            }
            if (it->second->Err != ErrNoError)
            {
                if (isRetriableControllerError(it->second->Err))
                {
                    std::shared_ptr<Broker> dummy;
                    RefreshController(dummy);
                }
                co_return it->second->Err;
            }
            co_return 0;
        });
}

coev::awaitable<int> ClusterAdmin::DescribeTopics(const std::vector<std::string> &topics,
                                                  std::vector<std::shared_ptr<TopicMetadata>> &out_metadata)
{
    std::shared_ptr<MetadataResponse> response;
    int err = co_await RetryOnError(
        isRetriableControllerError,
        [this, &topics, &response]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> controller;
            int err = co_await GetController(controller);
            if (err != 0)
            {
                co_return err;
            }
            auto request = NewMetadataRequest(conf_->Version, topics);
            err = co_await controller->GetMetadata(request, response);
            if (isRetriableControllerError(err))
            {
                std::shared_ptr<Broker> dummy;
                co_await RefreshController(dummy);
            }
            co_return err;
        });
    if (err != 0)
    {
        co_return err;
    }
    out_metadata = std::move(response->Topics);
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DescribeCluster(std::vector<std::shared_ptr<Broker>> &out_brokers, int32_t &out_controllerID)
{
    std::shared_ptr<MetadataResponse> response;
    int err = co_await RetryOnError(
        isRetriableControllerError,
        [this, &response]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> controller;
            int err = co_await GetController(controller);
            if (err != 0)
            {
                co_return err;
            }
            auto request = NewMetadataRequest(conf_->Version, std::vector<std::string>{});
            err = co_await controller->GetMetadata(request, response);
            if (isRetriableControllerError(err))
            {
                std::shared_ptr<Broker> dummy;
                RefreshController(dummy);
            }
            co_return err;
        });
    if (err != 0)
    {
        out_brokers.clear();
        out_controllerID = 0;
        co_return err;
    }
    out_brokers = std::move(response->Brokers);
    out_controllerID = std::move(response->ControllerID);
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::ListTopics(std::map<std::string, TopicDetail> &topicsDetailsMap)
{
    std::shared_ptr<Broker> b;
    int err = FindAnyBroker(b);
    if (err != 0)
    {
        co_return err;
    }
    err = co_await b->Open(conf_);
    auto metadataReq = NewMetadataRequest(conf_->Version, std::vector<std::string>{});
    std::shared_ptr<MetadataResponse> metadataResp;
    err = co_await b->GetMetadata(metadataReq, metadataResp);
    if (err != 0)
    {
        co_return err;
    }

    std::vector<std::shared_ptr<ConfigResource>> describeConfigsResources;
    for (auto &topic : metadataResp->Topics)
    {
        TopicDetail topicDetails;
        topicDetails.NumPartitions = static_cast<int32_t>(topic->Partitions.size());
        if (!topic->Partitions.empty())
        {
            topicDetails.ReplicaAssignment.clear();
            for (auto &partition : topic->Partitions)
            {
                topicDetails.ReplicaAssignment[partition->ID] = partition->Replicas;
            }
            topicDetails.ReplicationFactor = static_cast<int16_t>(topic->Partitions[0]->Replicas.size());
        }
        topicsDetailsMap[topic->Name] = topicDetails;
        auto topicResource = std::make_shared<ConfigResource>();
        topicResource->Type = TopicResource;
        topicResource->Name = topic->Name;
        describeConfigsResources.push_back(topicResource);
    }

    auto describeConfigsReq = std::make_shared<DescribeConfigsRequest>();
    describeConfigsReq->Resources = {describeConfigsResources};
    if (conf_->Version.IsAtLeast(V1_1_0_0))
    {
        describeConfigsReq->Version = 1;
    }
    if (conf_->Version.IsAtLeast(V2_0_0_0))
    {
        describeConfigsReq->Version = 2;
    }

    std::shared_ptr<DescribeConfigsResponse> describeConfigsResp;
    err = co_await b->DescribeConfigs(describeConfigsReq, describeConfigsResp);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &resource : describeConfigsResp->Resources)
    {
        auto it = topicsDetailsMap.find(resource->Name);
        if (it == topicsDetailsMap.end())
            continue;
        TopicDetail &topicDetails = it->second;
        topicDetails.ConfigEntries.clear();
        for (auto &entry : resource->Configs)
        {
            if (entry->Default || entry->Sensitive)
                continue;
            topicDetails.ConfigEntries[entry->Name] = entry->Value;
        }
        topicsDetailsMap[resource->Name] = topicDetails;
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
    request->Topics = {topic};
    request->Timeout = conf_->Admin.Timeout;
    if (conf_->Version.IsAtLeast(V2_4_0_0))
    {
        request->Version = 4;
    }
    else if (conf_->Version.IsAtLeast(V2_1_0_0))
    {
        request->Version = 3;
    }
    else if (conf_->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 2;
    }
    else if (conf_->Version.IsAtLeast(V0_11_0_0))
    {
        request->Version = 1;
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
            std::shared_ptr<DeleteTopicsResponse> rsp;
            err = co_await b->DeleteTopics(request, rsp);
            if (err != 0)
            {
                co_return err;
            }
            auto it = rsp->TopicErrorCodes.find(topic);
            if (it == rsp->TopicErrorCodes.end())
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
    tp->Count = count;
    tp->Assignment = assignment;
    topicPartitions[topic] = tp;

    auto request = std::make_shared<CreatePartitionsRequest>();
    request->TopicPartitions = topicPartitions;
    request->Timeout = conf_->Admin.Timeout;
    request->ValidateOnly = validateOnly;
    if (conf_->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 1;
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
            std::shared_ptr<CreatePartitionsResponse> rsp;
            err = co_await b->CreatePartitions(request, rsp);
            if (err != 0)
            {
                co_return err;
            }
            auto it = rsp->TopicPartitionErrors.find(topic);
            if (it == rsp->TopicPartitionErrors.end())
            {
                co_return ErrIncompleteResponse;
            }
            if (it->second->Err != ErrNoError)
            {
                if (it->second->Err == ErrNotController)
                {
                    std::shared_ptr<Broker> dummy;
                    RefreshController(dummy);
                }
                co_return it->second->Err;
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
    request->Timeout = std::chrono::milliseconds(60000);
    request->Version = 0;
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
            std::shared_ptr<AlterPartitionReassignmentsResponse> response;
            err = co_await b->AlterPartitionReassignments(request, response);
            std::vector<int> errs;
            if (err != 0)
            {
                errs.push_back(err);
            }
            else
            {
                if (response->ErrorCode > 0)
                {
                    errs.push_back(response->ErrorCode);
                }
                for (auto &topicErrors : response->Errors)
                {
                    for (auto &partitionError : topicErrors.second)
                    {
                        if (partitionError.second->errorCode != ErrNoError)
                        {
                            errs.push_back(partitionError.second->errorCode);
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
    request->Timeout = std::chrono::milliseconds(60000);
    request->Version = 0;
    request->AddBlock(topic, partitions);

    std::shared_ptr<ListPartitionReassignmentsResponse> response;
    int err = co_await RetryOnError(
        isRetriableControllerError,
        [this, &request, &response]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> b;
            int err = co_await GetController(b);
            if (err != 0)
            {
                co_return err;
            }
            err = co_await b->Open(client_->GetConfig());
            if (err != 0)
            {
                co_return err;
            }
            err = co_await b->ListPartitionReassignments(request, response);
            if (isRetriableControllerError(err))
            {
                std::shared_ptr<Broker> dummy;
                err = co_await RefreshController(dummy);
                if (err != 0)
                {
                    co_return err;
                }
            }
            co_return err;
        });

    if (err == 0 && response != nullptr)
    {
        out_topicStatus = response->TopicStatus;
        co_return 0;
    }
    else
    {
        out_topicStatus.clear();
        co_return err;
    }
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
        int err = co_await client_->Leader(topic, partition, broker);
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
        topicData->PartitionOffsets = std::move(recordsToDelete);
        topics[topic] = topicData;

        auto request = std::make_shared<DeleteRecordsRequest>();
        request->Topics = std::move(topics);
        request->Timeout = conf_->Admin.Timeout;
        if (conf_->Version.IsAtLeast(V2_0_0_0))
        {
            request->Version = 1;
        }

        std::shared_ptr<DeleteRecordsResponse> rsp;
        int err = co_await broker->DeleteRecords(request, rsp);
        if (err != 0)
        {
            errs.emplace_back(err);
            continue;
        }

        auto it = rsp->Topics.find(topic);
        if (it == rsp->Topics.end())
        {
            errs.emplace_back(ErrIncompleteResponse);
            continue;
        }

        for (auto &part : it->second->Partitions)
        {
            if (part.second->Err != ErrNoError)
            {
                errs.emplace_back(part.second->Err);
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
    request->Resources = resources;
    if (conf_->Version.IsAtLeast(V1_1_0_0))
    {
        request->Version = 1;
    }
    if (conf_->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 2;
    }

    std::shared_ptr<Broker> b;
    int err;
    if (dependsOnSpecificNode(resource))
    {

        int32_t id = static_cast<int32_t>(std::stol(resource.Name));
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
    err = co_await b->Open(client_->GetConfig());
    if (err != 0)
    {
        co_return err;
    }

    std::shared_ptr<DescribeConfigsResponse> rsp;
    err = co_await b->DescribeConfigs(request, rsp);
    if (err != 0)
    {
        co_return err;
    }

    out_entries.clear();
    for (auto &rspResource : rsp->Resources)
    {
        if (rspResource->Name == resource.Name)
        {
            if (rspResource->ErrorCode != 0)
            {
                co_return rspResource->ErrorCode;
            }
            for (auto &cfgEntry : rspResource->Configs)
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
    res->Type = resourceType;
    res->Name = name;
    res->ConfigEntries = entries;
    resources.push_back(res);

    auto request = std::make_shared<AlterConfigsRequest>();
    request->Resources = resources;
    request->ValidateOnly = validateOnly;
    if (conf_->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 1;
    }

    std::shared_ptr<Broker> b;
    int err;
    ConfigResource tmp = {
        .Type = resourceType,
        .Name = name,
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

    err = co_await b->Open(client_->GetConfig());
    if (err != 0)
    {
        co_return err;
    }

    std::shared_ptr<AlterConfigsResponse> rsp;
    err = co_await b->AlterConfigs(request, rsp);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &rspResource : rsp->Resources)
    {
        if (rspResource->Name == name)
        {
            if (rspResource->ErrorCode != 0)
            {
                co_return rspResource->ErrorCode;
            }
        }
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::IncrementalAlterConfig(ConfigResourceType resourceType, const std::string &name, const std::map<std::string, IncrementalAlterConfigsEntry> &entries, bool validateOnly)
{
    std::vector<std::shared_ptr<IncrementalAlterConfigsResource>> resources;
    auto res = std::make_shared<IncrementalAlterConfigsResource>();
    res->Type = resourceType;
    res->Name = name;
    res->ConfigEntries = entries;
    resources.push_back(res);

    auto request = std::make_shared<IncrementalAlterConfigsRequest>();
    request->Resources = resources;
    request->ValidateOnly = validateOnly;
    if (conf_->Version.IsAtLeast(V2_4_0_0))
    {
        request->Version = 1;
    }

    std::shared_ptr<Broker> b;
    int err;
    ConfigResource tmp = {
        .Type = resourceType,
        .Name = name,
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
    err = co_await b->Open(client_->GetConfig());

    std::shared_ptr<IncrementalAlterConfigsResponse> response;
    err = co_await b->IncrementalAlterConfigs(request, response);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &rspResource : response->Resources)
    {
        if (rspResource->Name == name)
        {
            if (rspResource->ErrorCode != static_cast<int16_t>(ErrNoError))
            {
                int err = rspResource->ErrorCode;
                if (!rspResource->ErrorMsg.empty())
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
    request->AclCreations = acls;
    if (conf_->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 1;
    }

    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    std::shared_ptr<CreateAclsResponse> rsp;
    co_return co_await b->CreateAcls(request, rsp);
}

coev::awaitable<int> ClusterAdmin::CreateACLs(const std::vector<std::shared_ptr<ResourceAcls>> &resourceACLs)
{
    std::vector<std::shared_ptr<AclCreation>> acls;
    for (auto &resourceACL : resourceACLs)
    {
        for (auto &acl : resourceACL->Acls)
        {
            acls.emplace_back(std::make_shared<AclCreation>(resourceACL->Resource_, *acl));
        }
    }

    auto request = std::make_shared<CreateAclsRequest>();
    request->AclCreations = std::move(acls);
    if (conf_->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 1;
    }

    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    std::shared_ptr<CreateAclsResponse> response;
    co_return co_await b->CreateAcls(request, response);
}

coev::awaitable<int> ClusterAdmin::ListAcls(const AclFilter &filter, std::vector<std::shared_ptr<ResourceAcls>> &out_results)
{
    auto request = std::make_shared<DescribeAclsRequest>();
    request->Filter = filter;
    if (conf_->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 1;
    }

    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    std::shared_ptr<DescribeAclsResponse> response;
    err = co_await b->DescribeAcls(request, response);
    if (err != 0)
    {
        co_return err;
    }

    out_results.clear();
    for (auto &rAcl : response->ResourceAcls_)
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
    request->Filters = filters;
    if (conf_->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 1;
    }

    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    std::shared_ptr<DeleteAclsResponse> rsp;
    err = co_await b->DeleteAcls(request, rsp);
    if (err != 0)
    {
        co_return err;
    }

    out_matchingAcls.clear();
    for (auto &fr : rsp->FilterResponses)
    {
        for (auto &mACL : fr->MatchingAcls)
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
    request->Type = electionType;
    request->TopicPartitions = partitions;
    request->Timeout = std::chrono::milliseconds(60000);
    if (conf_->Version.IsAtLeast(V2_4_0_0))
    {
        request->Version = 2;
    }
    else if (conf_->Version.IsAtLeast(V0_11_0_0))
    {
        request->Version = 1;
    }

    std::shared_ptr<ElectLeadersResponse> res;
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
            err = co_await b->Open(client_->GetConfig());
            if (err != 0)
            {
                co_return err;
            }
            err = co_await b->ElectLeaders(request, res);
            if (err != 0)
            {
                co_return err;
            }
            if (res->ErrorCode != ErrNoError)
            {
                if (isRetriableControllerError(res->ErrorCode))
                {
                    std::shared_ptr<Broker> dummy;
                    RefreshController(dummy);
                }
                co_return res->ErrorCode;
            }
            co_return 0;
        });

    if (err != 0)
    {
        out_results.clear();
        co_return err;
    }
    out_results = res->ReplicaElectionResults;
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DescribeConsumerGroups(const std::vector<std::string> &groups,
                                                          std::vector<std::shared_ptr<GroupDescription>> &out_result)
{
    std::map<std::shared_ptr<Broker>, std::vector<std::string>> groupsPerBroker;
    for (auto &group : groups)
    {
        std::shared_ptr<Broker> coordinator;
        int err = co_await client_->GetCoordinator(group, coordinator);
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
        describeReq->Groups = brokerGroups;
        if (conf_->Version.IsAtLeast(V2_4_0_0))
        {
            describeReq->Version = 5;
        }
        else if (conf_->Version.IsAtLeast(V2_3_0_0))
        {
            describeReq->Version = 3;
        }
        else if (conf_->Version.IsAtLeast(V2_0_0_0))
        {
            describeReq->Version = 2;
        }
        else if (conf_->Version.IsAtLeast(V1_1_0_0))
        {
            describeReq->Version = 1;
        }

        std::shared_ptr<DescribeGroupsResponse> response;
        int err = co_await broker->DescribeGroups(describeReq, response);
        if (err != 0)
        {
            co_return err;
        }
        out_result.insert(out_result.end(), response->Groups.begin(), response->Groups.end());
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::ListConsumerGroups(std::map<std::string, std::string> &out_groups)
{
    auto brokers = client_->Brokers();

    std::list<int> errChan;
    coev::co_task _task;
    for (auto &b : brokers)
    {
        _task << [this, b, &out_groups, &errChan]() -> coev::awaitable<int>
        {
            b->Open(conf_);
            auto request = std::make_shared<ListGroupsRequest>();
            if (conf_->Version.IsAtLeast(V3_8_0_0))
            {
                request->Version = 5;
            }
            else if (conf_->Version.IsAtLeast(V2_6_0_0))
            {
                request->Version = 4;
            }
            else if (conf_->Version.IsAtLeast(V2_4_0_0))
            {
                request->Version = 3;
            }
            else if (conf_->Version.IsAtLeast(V2_0_0_0))
            {
                request->Version = 2;
            }
            else if (conf_->Version.IsAtLeast(V0_11_0_0))
            {
                request->Version = 1;
            }
            std::shared_ptr<ListGroupsResponse> response;
            int err = co_await b->ListGroups(request, response);
            if (err != 0)
            {
                errChan.push_back(err);
                co_return err;
            }
            out_groups.insert(response->Groups.begin(), response->Groups.end());
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
    auto request = OffsetFetchRequest::NewOffsetFetchRequest(conf_->Version, group, topicPartitions);
    co_return co_await RetryOnError(
        isRetriableGroupCoordinatorError,
        [this, &request, &out, &group]() -> coev::awaitable<int>
        {
        std::shared_ptr<Broker> coordinator;
        int err =co_await client_->GetCoordinator(group, coordinator);
        if (err != 0) {
            co_return err;
        }
        err =co_await coordinator->FetchOffset(request, out);
        if (err != 0) {
            if (isRetriableGroupCoordinatorError(err)) 
            {
                client_->RefreshCoordinator(group);
            }
            co_return err;
        }
        if (out->Err != ErrNoError)
         {
            if (isRetriableGroupCoordinatorError(out->Err))
             {
                client_->RefreshCoordinator(group);
            }
            co_return out->Err;
        }
        co_return 0; });
}

coev::awaitable<int> ClusterAdmin::DeleteConsumerGroupOffset(const std::string &group, const std::string &topic, int32_t partition)
{
    auto request = std::make_shared<DeleteOffsetsRequest>();
    request->Group = group;
    request->partitions[topic] = {partition};

    co_return co_await RetryOnError(
        isRetriableGroupCoordinatorError,
        [this, &request, &group, &topic, &partition]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> coordinator;
            int err = co_await client_->GetCoordinator(group, coordinator);
            if (err != 0)
            {
                co_return err;
            }
            std::shared_ptr<DeleteOffsetsResponse> response;
            err = co_await coordinator->DeleteOffsets(request, response);
            if (err != 0)
            {
                if (isRetriableGroupCoordinatorError(err))
                {
                    client_->RefreshCoordinator(group);
                }
                co_return err;
            }
            if (response->ErrorCode != ErrNoError)
            {
                if (isRetriableGroupCoordinatorError(response->ErrorCode))
                {
                    client_->RefreshCoordinator(group);
                }
                co_return response->ErrorCode;
            }
            auto topicIt = response->Errors.find(topic);
            if (topicIt != response->Errors.end())
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
    request->Groups = {group};
    if (conf_->Version.IsAtLeast(V2_4_0_0))
    {
        request->Version = 2;
    }
    else if (conf_->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 1;
    }

    co_return co_await RetryOnError(
        isRetriableGroupCoordinatorError,
        [this, &request, &group]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> coordinator;
            int err = co_await client_->GetCoordinator(group, coordinator);
            if (err != 0)
            {
                co_return err;
            }
            std::shared_ptr<DeleteGroupsResponse> response;
            err = co_await coordinator->DeleteGroups(request, response);
            if (err != 0)
            {
                if (isRetriableGroupCoordinatorError(err))
                {
                    client_->RefreshCoordinator(group);
                }
                co_return err;
            }
            auto it = response->GroupErrorCodes.find(group);
            if (it == response->GroupErrorCodes.end())
            {
                co_return ErrIncompleteResponse;
            }
            if (it->second != ErrNoError)
            {
                if (isRetriableGroupCoordinatorError(it->second))
                {
                    err = co_await client_->RefreshCoordinator(group);
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
            Logger::Printf("Unable to find broker with ID = %d\n", b_id);
            continue;
        }
        task_ << [this](std::shared_ptr<Broker> broker, std::list<result> &logDirsResults, std::list<int> &errChan) -> coev::awaitable<void>
        {
            auto err = co_await broker->Open(conf_);
            if (err != 0)
            {
                errChan.push_back(err);
                co_return;
            }
            auto request = std::make_shared<DescribeLogDirsRequest>();
            if (conf_->Version.IsAtLeast(V3_3_0_0))
            {
                request->Version = 4;
            }
            else if (conf_->Version.IsAtLeast(V3_2_0_0))
            {
                request->Version = 3;
            }
            else if (conf_->Version.IsAtLeast(V2_6_0_0))
            {
                request->Version = 2;
            }
            else if (conf_->Version.IsAtLeast(V2_0_0_0))
            {
                request->Version = 1;
            }

            std::shared_ptr<DescribeLogDirsResponse> response;
            err = co_await broker->DescribeLogDirs(request, response);
            if (err != 0)
            {
                errChan.push_back(err);
                co_return;
            }
            if (response->ErrorCode != ErrNoError)
            {
                errChan.push_back(response->ErrorCode);
                co_return;
            }
            logDirsResults.emplace_back(broker->ID(), std::move(response->LogDirs));
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
        req->DescribeUsers.push_back(DescribeUserScramCredentialsRequestUser{u});
    }

    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    std::shared_ptr<DescribeUserScramCredentialsResponse> rsp;
    err = co_await b->DescribeUserScramCredentials(req, rsp);
    if (err != 0)
    {
        co_return err;
    }
    out_results = std::move(rsp->Results);
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
    req->Deletions = d;
    req->Upsertions = u;

    std::shared_ptr<AlterUserScramCredentialsResponse> rsp;
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
            err = co_await b->AlterUserScramCredentials(req, rsp);
            co_return err;
        });

    if (err != 0)
    {
        out_results.clear();
        co_return err;
    }
    out_results = std::move(rsp->Results);
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::DescribeClientQuotas(const std::vector<QuotaFilterComponent> &components, bool strict, std::vector<DescribeClientQuotasEntry> &out)
{
    auto request = NewDescribeClientQuotasRequest(conf_->Version, components, strict);
    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    std::shared_ptr<DescribeClientQuotasResponse> rsp;
    err = co_await b->DescribeClientQuotas(request, rsp);
    if (err != 0)
    {
        co_return err;
    }
    if (!rsp->ErrorMsg.empty())
    {
        co_return ErrDescribeClientQuotas;
    }
    if (rsp->ErrorCode != ErrNoError)
    {
        co_return rsp->ErrorCode;
    }
    out = std::move(rsp->Entries);
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::AlterClientQuotas(const std::vector<QuotaEntityComponent> &entity, const ClientQuotasOp &op, bool validateOnly)
{
    AlterClientQuotasEntry entry;
    entry.Entity = entity;
    entry.Ops = {op};

    auto request = std::make_shared<AlterClientQuotasRequest>();
    request->Entries = {entry};
    request->ValidateOnly = validateOnly;

    std::shared_ptr<Broker> b;
    int err = co_await GetController(b);
    if (err != 0)
    {
        co_return err;
    }
    std::shared_ptr<AlterClientQuotasResponse> rsp;
    err = co_await b->AlterClientQuotas(request, rsp);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &entry : rsp->Entries)
    {
        if (!entry.ErrorMsg.empty())
        {
            co_return ErrAlterClientQuotas;
        }
        if (entry.ErrorCode != ErrNoError)
        {
            co_return entry.ErrorCode;
        }
    }
    co_return 0;
}

coev::awaitable<int> ClusterAdmin::RemoveMemberFromConsumerGroup(const std::string &groupId, const std::vector<std::string> &groupInstanceIds, std::shared_ptr<LeaveGroupResponse> &out_response)
{
    if (!conf_->Version.IsAtLeast(V2_4_0_0))
    {
        co_return ErrUnsupportedVersion;
    }

    auto request = std::make_shared<LeaveGroupRequest>();
    request->Version = 3;
    request->GroupId = groupId;
    for (auto &instanceId : groupInstanceIds)
    {
        MemberIdentity member;
        member.GroupInstanceId = instanceId;
        request->Members.push_back(member);
    }

    co_return co_await RetryOnError(
        isRetriableGroupCoordinatorError,
        [this, &request, &groupId, &out_response]() -> coev::awaitable<int>
        {
            std::shared_ptr<Broker> coordinator;
            int err = co_await client_->GetCoordinator(groupId, coordinator);
            if (err != 0)
            {
                co_return err;
            }
            err = co_await coordinator->LeaveGroup(request, out_response);
            if (err != 0)
            {
                if (isRetriableGroupCoordinatorError(err))
                {
                    client_->RefreshCoordinator(groupId);
                }
                co_return err;
            }
            if (out_response->Err != ErrNoError)
            {
                if (isRetriableGroupCoordinatorError(out_response->Err))
                {
                    client_->RefreshCoordinator(groupId);
                }
                co_return out_response->Err;
            }
            co_return 0;
        });
}