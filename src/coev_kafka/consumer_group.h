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

struct ConsumerGroup : std::enable_shared_from_this<ConsumerGroup>
{
    std::shared_ptr<Client> m_client;
    std::shared_ptr<Config> m_config;
    std::shared_ptr<IConsumer> m_consumer;
    std::string m_group_id;
    std::string m_group_instance_id;
    std::string m_member_id;
    coev::co_channel<std::shared_ptr<ConsumerError>> m_errors;
    coev::co_channel<bool> m_closed;
    std::string m_user_data;

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
    coev::awaitable<int> JoinGroup(std::shared_ptr<Broker> coordinator, const std::vector<std::string> &topics, JoinGroupResponse &response);

    std::shared_ptr<BalanceStrategy> FindStrategy(const std::string &name, const std::vector<std::shared_ptr<BalanceStrategy>> &groupStrategies, bool &ok);

    coev::awaitable<int> SyncGroup(std::shared_ptr<Broker> coordinator, std::map<std::string, ConsumerGroupMemberMetadata> members,
                                   const BalanceStrategyPlan &plan, int32_t generationID, std::shared_ptr<BalanceStrategy> strategy, SyncGroupResponse &response);

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

coev::awaitable<int> NewConsumerGroup(const std::vector<std::string> &addrs, const std::string &group_id, std::shared_ptr<Config> config, std::shared_ptr<ConsumerGroup> &consumer_group);
coev::awaitable<int> NewConsumerGroupSession(std::shared_ptr<Context> &context, std::shared_ptr<ConsumerGroup> parent, std::map<std::string, std::vector<int32_t>> &claims, const std::string &memberID, int32_t generationID, std::shared_ptr<ConsumerGroupHandler> &handler, std::shared_ptr<ConsumerGroupSession> &session);