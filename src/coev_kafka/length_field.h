#pragma once

#include <cstdint>
#include <vector>
#include <stack>
#include <mutex>
#include <memory>
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "encoder_decoder.h"

struct LengthField : push_encoder, push_decoder
{
    int m_start_offset = 0;
    int32_t m_length = 0;

    LengthField() = default;
    int decode(packet_decoder &pd);
    void save_offset(int in);
    int reserve_length();

    int run(int cur_offset, std::string &buf);
    int check(int cur_offset, const std::string_view &buf);
};
