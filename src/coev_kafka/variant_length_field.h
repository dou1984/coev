#pragma once

#include "packet_encoder.h"
#include "packet_decoder.h"

struct VariantLengthField : dynamicPushDecoder, dynamicPushEncoder
{
    int m_start_offset = 0;
    int64_t m_length = 0;

    int decode(packetDecoder &pd);
    void saveOffset(int in);
    int reserveLength();
    int adjust_length(int currOffset);
    int run(int curOffset, std::string &buf);
    int check(int curOffset, const std::string &buf);
};