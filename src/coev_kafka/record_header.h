#pragma once
#include <vector>
#include "encoder_decoder.h"
#include "packet_decoder.h"
#include "packet_encoder.h"

struct RecordHeader : IEncoder, IDecoder
{
    std::string Key;
    std::string Value;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
};
