#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "packet_encoder.h"
#include "errors.h"

enum CrcPolynomial : int8_t
{
    CrcIEEE = 0,
    CrcCastagnoli = 1
};

struct crc32Field : pushEncoder
{
    crc32Field(CrcPolynomial polynomial);

    void saveOffset(int in);
    int reserveLength();
    int run(int curOffset, std::string &buf);
    int check(int curOffset, std::string &buf);
    int crc(int curOffset, const std::string &buf, uint32_t &out_crc);

    int startOffset;
    CrcPolynomial polynomial;
};
std::shared_ptr<crc32Field> acquireCrc32Field(CrcPolynomial polynomial);
void releaseCrc32Field(std::shared_ptr<crc32Field> c);