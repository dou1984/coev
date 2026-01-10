#pragma once
#include "errors.h"
#include "encoder_decoder.h"
#include "packet_decoder.h"
#include "packet_encoder.h"

struct PartitionError : IEncoder, VDecoder
{
    int32_t m_partition;
    KError m_err;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};
