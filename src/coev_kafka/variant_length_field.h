#pragma once

#include "packet_encoder.h"
#include "packet_decoder.h"

struct VariantLengthField : dynamicPushDecoder, dynamicPushEncoder
{
    int startOffset = 0;
    int64_t length = 0;

    int decode(PDecoder &pd);
    void saveOffset(int in);
    int reserveLength();
    int adjustLength(int currOffset);
    int run(int curOffset, std::string &buf);
    int check(int curOffset, const std::string &buf);
};