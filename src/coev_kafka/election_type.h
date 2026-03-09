/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <cstdint>

namespace coev::kafka
{
    enum ElectionType : int8_t
    {
        Preferred = 0,
        Unclean = 1
    };
}