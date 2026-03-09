/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <chrono>

namespace coev::kafka
{
    struct throttle_support
    {
        virtual std::chrono::milliseconds throttle_time() const = 0;
    };
}
