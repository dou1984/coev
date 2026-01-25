#pragma once

#include <cstdint>

#include "packet_decoder.h"
#include "version.h"

inline const int32_t MaxResponseSize = 100 * 1024 * 1024;
struct ResponseHeader : versioned_decoder
{

    int32_t m_length = 0;
    int32_t m_correlation_id = 0;

    int decode(packetDecoder &pd, int16_t version);
};
