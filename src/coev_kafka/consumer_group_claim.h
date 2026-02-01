#pragma once

#include <memory>
#include <queue>
#include "consumer.h"
#include "partition_consumer.h"

struct ConsumerGroupClaim final
{
    std::string m_topic;
    int32_t m_partition;
    int64_t m_offset;
    std::shared_ptr<PartitionConsumer> m_partition_consumer;
    coev::co_channel<std::shared_ptr<ConsumerMessage>> m_messages;
    coev::co_channel<std::shared_ptr<ConsumerError>> m_errors;

    ConsumerGroupClaim(const std::string &topic, int32_t partition, int64_t offset, std::shared_ptr<PartitionConsumer> partition_consumer);

    std::string topic();
    int32_t partition();
    int64_t initial_offset();
    int64_t high_water_mark_offset();

    coev::awaitable<int> messages(std::shared_ptr<ConsumerMessage> &);
    coev::awaitable<int> wait_closed(std::shared_ptr<ConsumerError> &);
};