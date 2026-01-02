#pragma once

#include <cstdint>

#include "packet_decoder.h"
#include "version.h"

inline const size_t MaxResponseSize = 100 * 1024 * 1024;
struct responseHeader : VDecoder
{

    int32_t length;
    int32_t correlationID;

    int decode(PDecoder &pd, int16_t version);
};
