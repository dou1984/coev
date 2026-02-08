#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <coev/coev.h>
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "topic_type.h"

struct StickyAssignorUserData
{

    virtual ~StickyAssignorUserData() = default;
    virtual std::vector<topic_t> partitions() = 0;
    virtual bool has_generation() = 0;
    virtual int generation() = 0;
};

struct StickyAssignorUserDataV0 : StickyAssignorUserData
{

    std::map<std::string, std::vector<int32_t>> m_topics;
    std::vector<topic_t> m_topic_partitions;

    int encode(packet_encoder &pe);
    int decode(packet_decoder &pd);

    std::vector<topic_t> partitions();
    bool has_generation();
    int generation();
};

struct StickyAssignorUserDataV1 : StickyAssignorUserData
{

    std::map<std::string, std::vector<int32_t>> m_topics;
    int32_t m_generation = 0;
    std::vector<topic_t> m_topic_partitions;

    int encode(packet_encoder &pe);
    int decode(packet_decoder &pd);

    std::vector<topic_t> partitions();
    bool has_generation();
    int generation();
};

std::vector<topic_t> PopulateTopicPartitions(const std::map<std::string, std::vector<int32_t>> &topics);