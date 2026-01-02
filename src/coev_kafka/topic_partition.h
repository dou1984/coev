#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "packet_decoder.h"
#include "packet_encoder.h"

struct TopicPartition
{
    int32_t Count;
    std::vector<std::vector<int32_t>> Assignment;
    std::string topic;
    int32_t partition;
    TopicPartition() = default;
    TopicPartition(const std::string &t, int32_t p);
    bool operator==(const TopicPartition &other) const { return topic == other.topic && partition == other.partition; }
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    struct Hash
    {
        std::size_t operator()(const TopicPartition &tp) const
        {
            return std::hash<std::string>()(tp.topic) ^ std::hash<int32_t>()(tp.partition);
        }
    };
};