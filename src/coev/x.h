/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

namespace coev
{
    template <typename T>
    T X(T &a)
    {
        T t = a;
        a = nullptr;
        return t;
    }
}