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
    enum ConfigSource : int8_t
    {
        SourceUnknown = 0,
        SourceTopic = 1,
        SourceDynamicBroker = 2,
        SourceDynamicDefaultBroker = 3,
        SourceStaticBroker = 4,
        SourceDefault = 5
    };
}