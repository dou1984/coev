#pragma once
#include <string>
#include "packet_decoder.h"
#include "packet_encoder.h"

struct PartitionOffsetMetadata : versioned_decoder, versioned_encoder
{
    int32_t m_partition;
    int64_t m_offset;
    int32_t m_leader_epoch;
    std::string m_metadata;

    int encode(packetEncoder &pe, int16_t version);
    int decode(packetDecoder &pd, int16_t version);
};