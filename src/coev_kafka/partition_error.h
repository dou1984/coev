/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include "errors.h"
#include "encoder_decoder.h"
#include "packet_decoder.h"
#include "packet_encoder.h"

namespace coev::kafka
{

    struct PartitionError : IEncoder, VDecoder
    {
        int32_t m_partition;
        KError m_err;

        int encode(PacketEncoder &pe) const;
        int decode(PacketDecoder &pd, int16_t version);
    };

} // namespace coev::kafka
