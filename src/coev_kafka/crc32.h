#pragma once
#include <vector>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "undefined.h"

struct CRC32 : pushEncoder, pushDecoder, Hash32
{
    CRC32() = default;
    CRC32(int)
    {
    }
    void saveOffset(int in)
    {
    }
    int reserveLength()
    {
        return 0;
    }

    int run(int curOffset, std::string &buf)
    {
        return 0;
    }
    int check(int curOffset, const std::string &buf)
    {
        return 0;
    }
};