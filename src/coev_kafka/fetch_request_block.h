#pragma once
#include <cstdint>
#include "encoder_decoder.h"
#include "packet_decoder.h"
#include "packet_encoder.h"

struct FetchRequestBlock : VDecoder, VEncoder
{
    int16_t Version;
    int32_t currentLeaderEpoch;
    int64_t fetchOffset;
    int64_t logStartOffset;
    int32_t maxBytes;

    FetchRequestBlock();
    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};
