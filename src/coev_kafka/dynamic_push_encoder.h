#pragma once
#include <vector>

struct push_encoder
{
    virtual ~push_encoder() = default;
    virtual void save_offset(int in) = 0;
    virtual int reserve_length() = 0;
    virtual int run(int curOffset, std::string &buf) = 0;
};

struct dynamicPushEncoder : push_encoder
{
    virtual int adjust_length(int currOffset) = 0;
};