#pragma once

#include "encoder_decoder.h"

struct pushDecoder
{
    virtual ~pushDecoder() = default;
    virtual void saveOffset(int in) = 0;
    virtual int reserveLength() = 0;
    virtual int check(int curOffset, const std::string &buf) = 0;
};
struct dynamicPushDecoder : pushDecoder, IDecoder
{
};