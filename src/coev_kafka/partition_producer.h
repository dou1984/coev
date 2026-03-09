/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <memory>
#include <coev/coev.h>
#include "async_producer.h"
#include "producer_message.h"
#include "broker_producer.h"

namespace coev::kafka
{
    struct PartitionProducer
    {
        PartitionProducer(std::shared_ptr<AsyncProducer> parent, const std::string &topic, int32_t partition);
        ~PartitionProducer();

        awaitable<int> init();
        awaitable<int> dispatch();
        awaitable<void> backoff(int retries);
        awaitable<int> update_leader_if_broker_producer_is_nil(std::shared_ptr<ProducerMessage> msg);
        awaitable<void> flush_retry_buffers();
        awaitable<int> update_leader();
        void new_high_watermark(int hwm);

        std::string m_topic;
        int32_t m_partition;
        std::shared_ptr<AsyncProducer> m_parent;
        coev::co_channel<std::shared_ptr<ProducerMessage>> m_input;
        std::vector<PartitionRetryState> m_retry_state;
        std::shared_ptr<Broker> m_leader;
        std::shared_ptr<BrokerProducer> m_broker_producer;
        int m_high_watermark;
        coev::co_task m_task;
    };
} // namespace coev::kafka
