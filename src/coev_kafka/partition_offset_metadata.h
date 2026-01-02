#pragma once
#include <string>
#include "packet_decoder.h"
#include "packet_encoder.h"

struct PartitionOffsetMetadata
{
    int32_t Partition;
    int64_t Offset;
    int32_t LeaderEpoch;
    std::string Metadata;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};