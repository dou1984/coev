#pragma once
#include <cstdint>
#include "encoder_decoder.h"
#include "packet_decoder.h"
#include "packet_encoder.h"

struct FetchRequestBlock : versioned_decoder, versioned_encoder
{
    int16_t m_version;
    int32_t m_current_leader_epoch;
    int64_t m_fetch_offset;
    int64_t m_log_start_offset;
    int32_t m_max_bytes;

    FetchRequestBlock();
    int encode(packetEncoder &pe, int16_t version);
    int decode(packetDecoder &pd, int16_t version);
};
