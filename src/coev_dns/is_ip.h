/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#pragma once
#include <string_view>

namespace coev
{
    bool is_ipv4(std::string_view view);
    bool is_ipv6(std::string_view view);
}