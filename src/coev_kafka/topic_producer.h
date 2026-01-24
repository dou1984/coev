#pragma once
#include <memory>
#include <string>
#include <map>
#include <deque>
#include <coev/coev.h>
#include "undefined.h"
#include "partition_producer.h"

struct AsyncProducer;
struct Broker;
struct ProducerMessage;
struct Partitioner;
struct TopicProducer
{
    TopicProducer(std::shared_ptr<AsyncProducer> parent, const std::string &topic);
    ~TopicProducer();
    coev::awaitable<int> dispatch(std::shared_ptr<ProducerMessage> &msg);
    coev::awaitable<int> partition_message(std::shared_ptr<ProducerMessage> msg);

    std::shared_ptr<AsyncProducer> m_parent;
    std::string m_topic;
    std::shared_ptr<Partitioner> m_partitioner;
    std::map<int32_t, std::shared_ptr<PartitionProducer>> m_handlers;
    co_task m_task;
};
