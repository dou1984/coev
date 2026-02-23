/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#pragma once
#include <cstdint>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef htonll
inline uint64_t htonll(uint64_t x)
{
    if (1 == htonl(1))
    {
        return x;
    }
    else
    {
        return ((uint64_t)htonl(x) << 32) | (uint64_t)htonl(x >> 32);
    }
}
#endif