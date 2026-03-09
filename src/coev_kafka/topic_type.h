/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <string>

namespace coev::kafka
{

    struct topic_t
    {
        std::string m_topic;
        int32_t m_partition = 0;
        topic_t() = default;
        topic_t(const std::string &topic, int32_t partition) : m_topic(topic), m_partition(partition)
        {
        }
        bool operator==(const topic_t &other) const;
        bool operator<(const topic_t &other) const;
    };

} // namespace coev::kafka

namespace std
{
    template <>
    struct hash<coev::kafka::topic_t>
    {
        size_t operator()(const coev::kafka::topic_t &o) const
        {
            auto h1 = hash<string>{}(o.m_topic);
            auto h2 = hash<int>{}(o.m_partition);
            return h1 ^ (h2 << 1 | h2 >> (sizeof(size_t) * 8 - 1));
        }
    };
}