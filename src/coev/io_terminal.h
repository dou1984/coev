/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

namespace coev
{
    void intercept_singal();
    void ignore_signal(int sign);
}