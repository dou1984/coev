#pragma once
#include <string>

struct TopicPartitionAssignment
{
    std::string m_topic;
    int32_t m_partition = 0;
    TopicPartitionAssignment(const std::string &topic, int32_t partition) : m_topic(topic), m_partition(partition)
    {
    }
    bool operator==(const TopicPartitionAssignment &other) const;
    bool operator<(const TopicPartitionAssignment &other) const;
};

namespace std
{
    template <>
    struct hash<TopicPartitionAssignment>
    {
        size_t operator()(const TopicPartitionAssignment &o) const
        {
            auto h1 = hash<string>{}(o.m_topic);
            auto h2 = hash<int>{}(o.m_partition);
            return h1 ^ (h2 << 1 | h2 >> (sizeof(size_t) * 8 - 1));
        }
    };
}