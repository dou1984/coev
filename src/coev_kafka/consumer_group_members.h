#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>

#include "packet_decoder.h"
#include "packet_encoder.h"
#include "protocol_body.h"

struct OwnedPartition;
struct ConsumerGroupMemberMetadata : IEncoder, IDecoder
{

    int16_t Version = 0;
    std::vector<std::string> Topics;
    std::string UserData;
    std::vector<std::shared_ptr<OwnedPartition>> OwnedPartitions;
    int32_t GenerationId = 0;
    std::string RackId;

    ConsumerGroupMemberMetadata() = default;
    ConsumerGroupMemberMetadata(int16_t v) : Version(v) {}

    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
};

struct OwnedPartition : IEncoder, IDecoder
{
    std::string topic;
    std::vector<int32_t> partitions;

    OwnedPartition() = default;
    OwnedPartition(const std::string &t, const std::vector<int32_t> &p) : topic(t), partitions(p) {}

    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
};

struct ConsumerGroupMemberAssignment : IEncoder, IDecoder
{
    int16_t Version = 0;
    std::map<std::string, std::vector<int32_t>> Topics;
    std::string UserData;

    ConsumerGroupMemberAssignment() = default;
    ConsumerGroupMemberAssignment(int16_t v) : Version(v) {}

    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
};
