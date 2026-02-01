#pragma once

#include "encoder_decoder.h"

struct push_decoder
{
    virtual ~push_decoder() = default;
    virtual void save_offset(int in) = 0;
    virtual int reserve_length() = 0;
    virtual int check(int curOffset, const std::string_view &buf) = 0;
};
struct dynamicPushDecoder : push_decoder, IDecoder
{
};