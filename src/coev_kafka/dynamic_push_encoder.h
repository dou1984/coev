#pragma once
#include <vector>

struct pushEncoder
{
    virtual ~pushEncoder() = default;
    virtual void saveOffset(int in) = 0;
    virtual int reserveLength() = 0;
    virtual int run(int curOffset, std::string &buf) = 0;
};

struct dynamicPushEncoder : pushEncoder
{
    virtual int adjustLength(int currOffset) = 0;
};