#pragma once
#include <memory>
#include <coev/coev.h>
#include "async_producer.h"
#include "producer_message.h"
#include "broker_producer.h"

struct PartitionProducer
{
    PartitionProducer(std::shared_ptr<AsyncProducer> parent, const std::string &topic, int32_t partition);
    ~PartitionProducer();

    coev::awaitable<int> init();
    coev::awaitable<int> dispatch(std::shared_ptr<ProducerMessage> &msg);

    coev::awaitable<void> backoff(int retries);
    coev::awaitable<int> update_leader_if_broker_producer_is_nil(std::shared_ptr<ProducerMessage> msg);
    coev::awaitable<void> flush_retry_buffers();
    coev::awaitable<int> update_leader();

    void new_high_watermark(int hwm);

    std::shared_ptr<AsyncProducer> m_parent;
    std::string m_topic;
    int32_t m_partition;
    // coev::co_channel<std::shared_ptr<ProducerMessage>> m_input;

    std::shared_ptr<Broker> m_leader;
    std::shared_ptr<BrokerProducer> m_broker_producer;

    int m_high_watermark;
    std::vector<PartitionRetryState> m_retry_state;
};
