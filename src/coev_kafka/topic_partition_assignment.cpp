#include "topic_partition_assignment.h"

bool TopicPartitionAssignment::operator==(const TopicPartitionAssignment &other) const
{
    return (m_topic == other.m_topic && m_partition == other.m_partition);
}

bool TopicPartitionAssignment::operator<(const TopicPartitionAssignment &other) const
{
    return (m_topic < other.m_topic || (m_topic == other.m_topic && m_partition < other.m_partition));
}