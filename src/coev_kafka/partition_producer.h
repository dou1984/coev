#pragma once
#include <memory>
#include <coev/coev.h>
#include "async_producer.h"
#include "producer_message.h"
#include "broker_producer.h"

struct PartitionProducer
{
    PartitionProducer(std::shared_ptr<AsyncProducer> parent, const std::string &topic, int32_t partition);
    coev::awaitable<int> Dispatch();

    coev::awaitable<void> Backoff(int retries);
    coev::awaitable<int> UpdateLeaderIfBrokerProducerIsNil(std::shared_ptr<ProducerMessage> msg);
    coev::awaitable<void> FlushRetryBuffers();
    coev::awaitable<int> UpdateLeader();

    void NewHighWatermark(int hwm);
    std::shared_ptr<AsyncProducer> Parent;
    std::string Topic;
    int32_t Partition;
    coev::co_channel<std::shared_ptr<ProducerMessage>> Input;

    std::shared_ptr<Broker> Leader;
    std::shared_ptr<Breaker> Breaker_;
    std::shared_ptr<BrokerProducer> BrokerProducer_;

    int HighWatermark_;
    std::vector<PartitionRetryState> RetryState_;
};
