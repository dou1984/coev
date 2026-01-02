#pragma once
#include <string>

struct TopicPartitionAssignment
{
    std::string Topic;
    int32_t Partition;
    TopicPartitionAssignment(const std::string &topic, int32_t partition) ;
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
            auto h1 = hash<string>{}(o.Topic);
            auto h2 = hash<int>{}(o.Partition);
            return h1 ^ (h2 << 1 | h2 >> (sizeof(size_t) * 8 - 1));
        }
    };
}