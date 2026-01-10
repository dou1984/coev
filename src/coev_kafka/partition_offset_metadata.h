#pragma once
#include <string>
#include "packet_decoder.h"
#include "packet_encoder.h"

struct PartitionOffsetMetadata
{ 
    int32_t m_partition;
    int64_t m_offset;
    int32_t m_leader_epoch;
    std::string m_metadata;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};