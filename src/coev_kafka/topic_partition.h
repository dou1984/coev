#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "topic_type.h"

struct TopicPartition : topic_t
{
    int32_t m_count;
    std::vector<std::vector<int32_t>> m_assignment;

    TopicPartition() = default;
    TopicPartition(const std::string &t, int32_t p);
    bool operator==(const TopicPartition &other) const;
    int encode(packet_encoder &pe);
    int decode(packet_decoder &pd, int16_t version);
    struct Hash
    {
        std::size_t operator()(const TopicPartition &tp) const
        {
            return std::hash<std::string>()(tp.m_topic) ^ std::hash<int32_t>()(tp.m_partition);
        }
    };
};