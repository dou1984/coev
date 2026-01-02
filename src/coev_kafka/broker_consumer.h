#pragma once
#include <memory>
#include <coev.h>
#include "consumer.h"
#include "broker.h"

struct BrokerConsumer : IConsumer, std::enable_shared_from_this<BrokerConsumer>
{

    std::shared_ptr<Consumer> consumer_;
    std::shared_ptr<Broker> broker;

    std::unordered_map<std::shared_ptr<PartitionConsumer>, bool> subscriptions;
    coev::co_channel<std::shared_ptr<PartitionConsumer>> input;
    coev::co_channel<std::vector<std::shared_ptr<PartitionConsumer>>> newSubscriptions;
    coev::co_task task_;

    coev::co_waitgroup acks;
    int refs = 0;
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

    coev::awaitable<void> subscriptionManager();
    coev::awaitable<void> subscriptionConsumer();

    void updateSubscriptions(const std::vector<std::shared_ptr<PartitionConsumer>> &newSubscriptions);
    coev::awaitable<void> handleResponses();
    coev::awaitable<void> abort(int err);
    coev::awaitable<int> fetchNewMessages(std::shared_ptr<FetchResponse> &response);
};
std::shared_ptr<BrokerConsumer> NewBrokerConsumer(std::shared_ptr<Consumer> c, std::shared_ptr<Broker> broker);