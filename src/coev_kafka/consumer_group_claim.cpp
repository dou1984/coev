#include "consumer_group_claim.h"

ConsumerGroupClaim::ConsumerGroupClaim(const std::string &topic, int32_t partition, int64_t offset,
                                       std::shared_ptr<PartitionConsumer> partition_consumer) : topic(topic), partition(partition), offset(offset), partition_consumer(partition_consumer)
{
}

std::string ConsumerGroupClaim::Topic()
{
    return topic;
}
int32_t ConsumerGroupClaim::Partition()
{
    return partition;
}
int64_t ConsumerGroupClaim::InitialOffset()
{
    return offset;
}
int64_t ConsumerGroupClaim::HighWaterMarkOffset()
{
    return partition_consumer->HighWaterMarkOffset();
}

coev::awaitable<int> ConsumerGroupClaim::Messages(std::shared_ptr<ConsumerMessage> &msg)
{
    msg = co_await messages.get();
    co_return 0;
}
coev::awaitable<int> ConsumerGroupClaim::waitClosed(std::shared_ptr<ConsumerError> &err)
{
    err = co_await errors.get();
    co_return 0;
}