#include "consumer_group_claim.h"

ConsumerGroupClaim::ConsumerGroupClaim(const std::string &topic, int32_t partition, int64_t offset, std::shared_ptr<PartitionConsumer> partition_consumer) : m_topic(topic), m_partition(partition), m_offset(offset), m_partition_consumer(partition_consumer)
{
}

std::string ConsumerGroupClaim::topic()
{
    return m_topic;
}
int32_t ConsumerGroupClaim::partition()
{
    return m_partition;
}
int64_t ConsumerGroupClaim::initial_offset()
{
    return m_offset;
}
int64_t ConsumerGroupClaim::high_water_mark_offset()
{
    return m_partition_consumer->HighWaterMarkOffset();
}

coev::awaitable<int> ConsumerGroupClaim::messages(std::shared_ptr<ConsumerMessage> &msg)
{
    co_await m_messages.get(msg);
    co_return 0;
}
coev::awaitable<int> ConsumerGroupClaim::wait_closed(std::shared_ptr<ConsumerError> &err)
{
    co_await m_errors.get(err);
    co_return 0;
}