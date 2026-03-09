/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include "packet_encoder.h"
#include "packet_decoder.h"
#include <string_view>

namespace coev::kafka
{

    struct VariantLengthField : dynamicPushDecoder, dynamicPushEncoder
    {
        VariantLengthField() = default;
        int m_start_offset = 0;
        int64_t m_length = 0;

        int decode(packet_decoder &pd);
        void save_offset(int in);
        int reserve_length();
        int adjust_length(int currOffset);
        int run(int curOffset, std::string &buf);
        int check(int curOffset, const std::string_view &buf);
    };

} // namespace coev::kafka