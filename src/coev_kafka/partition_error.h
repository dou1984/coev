#pragma once
#include "errors.h"
#include "encoder_decoder.h"
#include "packet_decoder.h"
#include "packet_encoder.h"

struct PartitionError : IEncoder, versioned_decoder
{
    int32_t m_partition;
    KError m_err;

    int encode(packetEncoder &pe) const;
    int decode(packetDecoder &pd, int16_t version);
};
