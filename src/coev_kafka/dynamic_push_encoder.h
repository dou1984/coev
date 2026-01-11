#pragma once
#include <vector>

struct pushEncoder
{
    virtual ~pushEncoder() = default;
    virtual void save_offset(int in) = 0;
    virtual int reserve_length() = 0;
    virtual int run(int curOffset, std::string &buf) = 0;
};

struct dynamicPushEncoder : pushEncoder
{
    virtual int adjust_length(int currOffset) = 0;
};