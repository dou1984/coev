#pragma once
#include <memory>
#include <string>
#include <map>
#include <coev/coev.h>
#include "undefined.h"

struct AsyncProducer;
struct Broker;
struct ProducerMessage;
struct Partitioner;
struct TopicProducer
{
    TopicProducer(std::shared_ptr<AsyncProducer> parent, const std::string &topic);
    coev::awaitable<void> dispatch();

    coev::awaitable<int> partitionMessage(std::shared_ptr<ProducerMessage> msg);

    std::shared_ptr<AsyncProducer> parent_;
    std::string topic_;
    coev::co_channel<std::shared_ptr<ProducerMessage>> input_;
    std::shared_ptr<Breaker> breaker_;
    std::map<int32_t, coev::co_channel<std::shared_ptr<ProducerMessage>>> handlers_;
    std::shared_ptr<Partitioner> partitioner_;
};
