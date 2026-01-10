#pragma once

#include <cstdint>
#include <vector>
#include <stack>
#include <mutex>
#include <memory>
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "encoder_decoder.h"

struct LengthField : pushEncoder
{
    int m_start_offset = 0;
    int32_t m_length = 0;

    LengthField() = default;
    int decode(PDecoder &pd);
    void saveOffset(int in);
    int reserveLength();

    int run(int curOffset, std::string &buf);
    int check(int curOffset, const std::string &buf);
};



std::shared_ptr<LengthField> acquireLengthField();
void releaseLengthField(std::shared_ptr<LengthField> ptr);
