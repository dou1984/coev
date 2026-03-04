/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <string>

using QuotaEntityType = std::string;

enum QuotaMatchType : int8_t
{
    QuotaMatchExact = 0,
    QuotaMatchDefault = 1,
    QuotaMatchAny = 2
};

const QuotaEntityType QuotaEntityUser = "user";
const QuotaEntityType QuotaEntityClientID = "client-id";
const QuotaEntityType QuotaEntityIP = "ip";
