#pragma once
#include <arpa/inet.h>

#ifndef htonll
template <class T>
constexpr T htonll(T x)
{
    static_assert(sizeof(T) == (sizeof(uint32_t) * 2));
    return (1 == htonl(1)) ? x : ((T)htonl(x & 0xFFFFFFFF) << 32) + htonl(x >> 32);
}
#endif
