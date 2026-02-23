/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#pragma once
#include <cstdint>
#include <arpa/inet.h>

namespace coev::details
{
    union block
    {
        char *str;
        const char *c_str;
        uint32_t *u32;

        void set_size(uint32_t val);
        uint32_t get_size();
        uint32_t size() const;
        char *body();
        char *next();
    };
}