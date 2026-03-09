/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <memory>
#include <string>
#include <map>
#include <deque>
#include <coev/coev.h>
#include "undefined.h"
#include "partition_producer.h"

namespace coev::kafka
{
    struct AsyncProducer;
    struct ProducerMessage;
    struct Partitioner;

    struct TopicProducer
    {
        TopicProducer(std::shared_ptr<AsyncProducer> parent, const std::string &topic);
        ~TopicProducer();
        awaitable<void> dispatch();
        awaitable<int> partition_message(std::shared_ptr<ProducerMessage> &msg);

        std::string m_topic;
        std::shared_ptr<AsyncProducer> m_parent;
        std::shared_ptr<Partitioner> m_partitioner;
        std::map<int32_t, std::shared_ptr<PartitionProducer>> m_handlers;
        coev::co_channel<std::shared_ptr<ProducerMessage>> m_input;
        coev::co_task m_task;
    };
}