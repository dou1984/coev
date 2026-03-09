/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

namespace coev::kafka
{

    enum IsolationLevel
    {
        ReadUncommitted = 0,
        ReadCommitted = 1
    };

} // namespace coev::kafka
