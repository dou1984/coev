#pragma once
#include <memory>
#include <coev/coev.h>
#include "consumer.h"
#include "broker.h"

struct BrokerConsumer : IConsumer, std::enable_shared_from_this<BrokerConsumer>
{

    std::shared_ptr<Consumer> m_consumer;
    std::shared_ptr<Broker> m_broker;

    std::unordered_map<std::shared_ptr<PartitionConsumer>, bool> m_subscriptions;
    coev::co_channel<std::shared_ptr<PartitionConsumer>> m_input;
    coev::co_channel<std::vector<std::shared_ptr<PartitionConsumer>>> m_new_subscriptions;
    coev::co_task m_task;

    coev::co_waitgroup m_acks;
    int m_refs = 0;
    BrokerConsumer() = default;
    virtual ~BrokerConsumer() = default;

    int Topics(std::vector<std::string> &out);
    coev::awaitable<int> Partitions(const std::string &topic, std::vector<int32_t> &out);
    coev::awaitable<int> Close();
    std::map<std::string, std::map<int32_t, int64_t>> HighWaterMarks();

    void Pause(const std::map<std::string, std::vector<int32_t>> &topicPartitions);
    void Resume(const std::map<std::string, std::vector<int32_t>> &topicPartitions);
    void PauseAll();
    void ResumeAll();

    coev::awaitable<void> SubscriptionManager();
    coev::awaitable<void> SubscriptionConsumer();

    void UpdateSubscriptions(const std::vector<std::shared_ptr<PartitionConsumer>> &newSubscriptions);
    coev::awaitable<void> HandleResponses();
    coev::awaitable<void> Abort(int err);
    coev::awaitable<int> FetchNewMessages(std::shared_ptr<FetchResponse> &response);
};
std::shared_ptr<BrokerConsumer> NewBrokerConsumer(std::shared_ptr<Consumer> c, std::shared_ptr<Broker> broker);