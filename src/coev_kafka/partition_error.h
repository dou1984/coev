#pragma once
#include "errors.h"
#include "encoder_decoder.h"
#include "packet_decoder.h"
#include "packet_encoder.h"

struct PartitionError : IEncoder, VDecoder
{
    int32_t m_partition;
    KError m_err;

    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
};
