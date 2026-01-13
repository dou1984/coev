#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "packet_decoder.h"
#include "packet_encoder.h"

struct TopicPartition
{
    int32_t m_count;
    std::vector<std::vector<int32_t>> m_assignment;
    std::string m_topic;
    int32_t m_partition;
    TopicPartition() = default;
    TopicPartition(const std::string &t, int32_t p);
    bool operator==(const TopicPartition &other) const { return m_topic == other.m_topic && m_partition == other.m_partition; }
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    struct Hash
    {
        std::size_t operator()(const TopicPartition &tp) const
        {
            return std::hash<std::string>()(tp.m_topic) ^ std::hash<int32_t>()(tp.m_partition);
        }
    };
};