/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "sleep_for.h"

namespace coev::kafka
{
    awaitable<void> sleep_for(std::chrono::milliseconds ms)
    {
        std::chrono::microseconds us = std::chrono::duration_cast<std::chrono::microseconds>(ms);
        return coev::usleep_for(us.count());
    }
    awaitable<void> sleep_for(std::chrono::seconds s)
    {
        return coev::sleep_for(s.count());
    }
}