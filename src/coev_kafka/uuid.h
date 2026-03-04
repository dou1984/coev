/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <array>
#include <string>
#include <cstdint>

struct Uuid
{
    std::array<uint8_t, 16> data;
    std::string String() const;
};