#include "topic_partition_assignment.h"

TopicPartitionAssignment::TopicPartitionAssignment(const std::string &topic, int32_t partition) : Topic(topic), Partition(partition)
{
}
bool TopicPartitionAssignment::operator==(const TopicPartitionAssignment &other) const
{
    return (Topic == other.Topic && Partition == other.Partition);
}
bool TopicPartitionAssignment::operator<(const TopicPartitionAssignment &other) const
{
    return (Topic < other.Topic || (Topic == other.Topic && Partition < other.Partition));
}