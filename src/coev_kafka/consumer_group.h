#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>
#include <queue>
#include <shared_mutex>
#include <coev/coev.h>
#include "client.h"
#include "consumer.h"
#include "broker.h"
#include "balance_strategy.h"
#include "consumer_group_members.h"
#include "join_group_request.h"
#include "join_group_response.h"
#include "sync_group_response.h"
#include "heartbeat_response.h"
#include "leave_group_response.h"
#include "consumer_group_handler.h"
#include "undefined.h"

struct PartitionConsumer;
struct ConsumerGroupSession;

struct IConsumerGroup
{
    virtual ~IConsumerGroup() = default;
    virtual coev::awaitable<int> Consume(std::shared_ptr<Context> &ctx, const std::vector<std::string> &topics, std::shared_ptr<ConsumerGroupHandler> handler) = 0;
    virtual coev::awaitable<int> Close() = 0;
    virtual coev::co_channel<std::shared_ptr<ConsumerError>> &Errors() = 0;
    virtual void Pause(const std::map<std::string, std::vector<int32_t>> &partitions) = 0;
    virtual void Resume(const std::map<std::string, std::vector<int32_t>> &partitions) = 0;
    virtual void PauseAll() = 0;
    virtual void ResumeAll() = 0;
};

struct ConsumerGroup : IConsumerGroup, std::enable_shared_from_this<ConsumerGroup>
{
    std::shared_ptr<Client> m_client;
    std::shared_ptr<Config> m_config;
    std::shared_ptr<IConsumer> m_consumer;
    std::string m_group_id;
    std::string m_group_instance_id;
    std::string m_member_id;
    coev::co_channel<std::shared_ptr<ConsumerError>> m_errors;

    mutable std::mutex m_lock;
    mutable std::shared_mutex m_errors_lock;
    coev::co_channel<bool> m_closed;
    std::string m_user_data;

    std::shared_ptr<metrics::Registry> m_metric_registry;
    std::shared_ptr<Broker> m_coordinator;
    coev::co_task m_task;

    ConsumerGroup(std::shared_ptr<Client> client, std::shared_ptr<IConsumer> consumer, std::shared_ptr<Config> config, const std::string &group_id);

    coev::awaitable<int> Consume(std::shared_ptr<Context> &ctx, const std::vector<std::string> &topics, std::shared_ptr<ConsumerGroupHandler> handler);
    coev::awaitable<int> Close();
    coev::co_channel<std::shared_ptr<ConsumerError>> &Errors();
    void Pause(const std::map<std::string, std::vector<int32_t>> &partitions);
    void Resume(const std::map<std::string, std::vector<int32_t>> &partitions);
    void PauseAll();
    void ResumeAll();

    coev::awaitable<int> NewSession(std::shared_ptr<Context> &ctx, const std::vector<std::string> &topics, std::shared_ptr<ConsumerGroupHandler> handler, int retries, std::shared_ptr<ConsumerGroupSession> &session);
    coev::awaitable<int> RetryNewSession(std::shared_ptr<Context> &ctx, const std::vector<std::string> &topics, std::shared_ptr<ConsumerGroupHandler> handler, int retries, bool refreshCoordinator, std::shared_ptr<ConsumerGroupSession> &out);
    coev::awaitable<int> JoinGroup(std::shared_ptr<Broker> coordinator, const std::vector<std::string> &topics, std::shared_ptr<JoinGroupResponse> &response);

    std::shared_ptr<BalanceStrategy> findStrategy(const std::string &name, const std::vector<std::shared_ptr<BalanceStrategy>> &groupStrategies, bool &ok);

    coev::awaitable<int> SyncGroup(std::shared_ptr<Broker> coordinator, std::map<std::string, ConsumerGroupMemberMetadata> members,
                                   const BalanceStrategyPlan &plan, int32_t generationID, std::shared_ptr<BalanceStrategy> strategy, std::shared_ptr<SyncGroupResponse> &response);

    coev::awaitable<int> Heartbeat(std::shared_ptr<Broker> coordinator, const std::string &memberID, int32_t generationID,
                                   std::shared_ptr<HeartbeatResponse> &response);

    coev::awaitable<int> Balance(std::shared_ptr<BalanceStrategy> strategy, const std::map<std::string, ConsumerGroupMemberMetadata> &members,
                                 std::map<std::string, std::vector<int32_t>> &topicPartitions, std::vector<std::string> &allSubscribedTopics, BalanceStrategyPlan &plan);
    coev::awaitable<int> Leave();

    coev::awaitable<void> LoopCheckPartitionNumbers(const std::map<std::string, std::vector<int32_t>> &allSubscribedTopicPartitions,
                                                    const std::vector<std::string> &topics, std::shared_ptr<ConsumerGroupSession> &session);
    coev::awaitable<int> TopicToPartitionNumbers(const std::vector<std::string> &topics, std::map<std::string, int> &topicToPartitionNum);

    void HandleError(std::shared_ptr<ConsumerError> err, const std::string &topic, int32_t partition);
};

std::shared_ptr<IConsumerGroup> NewConsumerGroup(const std::vector<std::string> &addrs, const std::string &groupID, std::shared_ptr<Config> config);
std::shared_ptr<IConsumerGroup> NewConsumerGroupFromClient(const std::string &groupID, std::shared_ptr<Client> client);
coev::awaitable<int> NewConsumerGroupSession(std::shared_ptr<Context> context, std::shared_ptr<ConsumerGroup> parent, std::map<std::string, std::vector<int32_t>> &claims, const std::string &memberID, int32_t generationID, std::shared_ptr<ConsumerGroupHandler> &handler, std::shared_ptr<ConsumerGroupSession> &session);