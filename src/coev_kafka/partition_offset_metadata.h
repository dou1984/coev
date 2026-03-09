/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <string>
#include "packet_decoder.h"
#include "packet_encoder.h"

namespace coev::kafka
{

    struct PartitionOffsetMetadata : VDecoder, VEncoder
    {
        int32_t m_partition;
        int64_t m_offset;
        int32_t m_leader_epoch;
        std::string m_metadata;
        PartitionOffsetMetadata() = default;
        PartitionOffsetMetadata(int32_t partition, int64_t offset, int32_t leader_epoch, const std::string &metadata);
        int encode(packet_encoder &pe, int16_t version) const;
        int decode(packet_decoder &pd, int16_t version);
    };

} // namespace coev::kafka