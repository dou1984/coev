#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <coev/coev.h>
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "topic_partition_assignment.h"

struct StickyAssignorUserData
{

    virtual ~StickyAssignorUserData() = default;
    virtual std::vector<TopicPartitionAssignment> partitions() = 0;
    virtual bool hasGeneration() = 0;
    virtual int generation() = 0;
};

struct StickyAssignorUserDataV0 : StickyAssignorUserData
{

    std::map<std::string, std::vector<int32_t>> Topics;
    std::vector<TopicPartitionAssignment> topicPartitions;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd);

    std::vector<TopicPartitionAssignment> partitions();
    bool hasGeneration();
    int generation();
};

struct StickyAssignorUserDataV1 : StickyAssignorUserData
{

    std::map<std::string, std::vector<int32_t>> Topics;
    int32_t Generation;
    std::vector<TopicPartitionAssignment> topicPartitions;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd);

    std::vector<TopicPartitionAssignment> partitions();
    bool hasGeneration();
    int generation();
};

std::vector<TopicPartitionAssignment> populateTopicPartitions(const std::map<std::string, std::vector<int32_t>> &topics);