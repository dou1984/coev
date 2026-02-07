#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "packet_encoder.h"
#include "dynamic_push_decoder.h"
#include "errors.h"

enum CrcPolynomial : int8_t
{
    CrcIEEE = 0,
    CrcCastagnoli = 1
};

struct crc32_field : push_encoder, push_decoder
{
    crc32_field(CrcPolynomial polynomial);

    void save_offset(int in);
    int reserve_length();
    int run(int curOffset, std::string &buf);
    int check(int curOffset, const std::string_view &buf);
    int crc(int curOffset, const std::string_view &buf, uint32_t &out_crc);

    int m_start_offset;
    CrcPolynomial m_polynomial;
};
std::shared_ptr<crc32_field> acquire_crc32_field(CrcPolynomial polynomial);
void release_crc32_field(std::shared_ptr<crc32_field> c);