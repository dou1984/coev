/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "packet_encoder.h"
#include "dynamic_push_decoder.h"
#include "errors.h"

namespace coev::kafka
{
    enum CrcPolynomial : int8_t
    {
        CrcIEEE = 0,
        CrcCastagnoli = 1
    };

    struct Crc32Field : push_encoder, push_decoder
    {
        Crc32Field(CrcPolynomial polynomial);

        void save_offset(int in);
        int reserve_length();
        int run(int curOffset, std::string &buf);
        int check(int curOffset, const std::string_view &buf);
        int crc(int curOffset, const std::string_view &buf, uint32_t &out_crc);

        int m_start_offset;
        CrcPolynomial m_polynomial;
    };
    std::shared_ptr<Crc32Field> acquire_crc32_field(CrcPolynomial polynomial);
    void release_crc32_field(std::shared_ptr<Crc32Field> c);
}