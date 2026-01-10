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
    std::shared_ptr<AsyncProducer> m_parent;
    std::string m_topic;
    int32_t m_partition;
    coev::co_channel<std::shared_ptr<ProducerMessage>> m_input;

    std::shared_ptr<Broker> m_leader;
    std::shared_ptr<Breaker> m_breaker;
    std::shared_ptr<BrokerProducer> m_broker_producer;

    int m_high_watermark;
    std::vector<PartitionRetryState> m_retry_state;
};
