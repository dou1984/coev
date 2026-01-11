#pragma once

#include "encoder_decoder.h"

struct pushDecoder
{
    virtual ~pushDecoder() = default;
    virtual void save_offset(int in) = 0;
    virtual int reserve_length() = 0;
    virtual int check(int curOffset, const std::string &buf) = 0;
};
struct dynamicPushDecoder : pushDecoder, IDecoder
{
};