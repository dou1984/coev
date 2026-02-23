/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <cstring>
#include <coev/coev.h>
#include "is_ip.h"

namespace coev
{
    awaitable<int> parse_dns(const std::string &url, std::string &addr);
}