/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <memory>
#include <unordered_set>
#include <coev/coev.h>
#include "consumer.h"
#include "broker.h"

namespace coev::kafka
{

    struct BrokerConsumer : IConsumer, std::enable_shared_from_this<BrokerConsumer>
    {

        std::shared_ptr<Consumer> m_consumer;
        std::shared_ptr<Broker> m_broker;

        std::unordered_set<std::shared_ptr<PartitionConsumer>> m_subscriptions;
        std::vector<std::shared_ptr<PartitionConsumer>> m_new_subscriptions;
        coev::co_channel<std::shared_ptr<PartitionConsumer>> m_input;
        coev::co_task m_task;
        BrokerConsumer();
        BrokerConsumer(std::shared_ptr<Consumer> c, std::shared_ptr<Broker> broker);
        virtual ~BrokerConsumer() = default;

        int Topics(std::vector<std::string> &out);
        awaitable<int> Partitions(const std::string &topic, std::vector<int32_t> &out);
        awaitable<int> Close();
        std::map<std::string, std::map<int32_t, int64_t>> HighWaterMarks();

        void Pause(const std::map<std::string, std::vector<int32_t>> &topicPartitions);
        void Resume(const std::map<std::string, std::vector<int32_t>> &topicPartitions);
        void PauseAll();
        void ResumeAll();

        awaitable<void> SubscriptionManager();
        awaitable<void> SubscriptionConsumer();

        void UpdateSubscriptions();
        awaitable<void> HandleResponses();
        awaitable<void> Abort(int err);
        awaitable<int> FetchNewMessages(std::shared_ptr<FetchResponse> &response);
    };

} // namespace coev::kafka
