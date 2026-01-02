#pragma once

#include <memory>
#include <queue>
#include "consumer.h"
#include "partition_consumer.h"

struct IConsumerGroupClaim
{
    virtual ~IConsumerGroupClaim() = default;
    virtual std::string Topic() = 0;
    virtual int32_t Partition() = 0;
    virtual int64_t InitialOffset() = 0;
    virtual int64_t HighWaterMarkOffset() = 0;
    virtual coev::awaitable<int> Messages(std::shared_ptr<ConsumerMessage> &) = 0;
};

struct ConsumerGroupClaim : IConsumerGroupClaim
{
    std::string topic;
    int32_t partition;
    int64_t offset;
    std::shared_ptr<PartitionConsumer> partition_consumer;
    coev::co_channel<std::shared_ptr<ConsumerMessage>> messages;
    coev::co_channel<std::shared_ptr<ConsumerError>> errors;

    ConsumerGroupClaim(const std::string &topic, int32_t partition, int64_t offset, std::shared_ptr<PartitionConsumer> partition_consumer);

    std::string Topic();
    int32_t Partition();
    int64_t InitialOffset();
    int64_t HighWaterMarkOffset();

    coev::awaitable<int> Messages(std::shared_ptr<ConsumerMessage> &);
    coev::awaitable<int> waitClosed(std::shared_ptr<ConsumerError> &);
};