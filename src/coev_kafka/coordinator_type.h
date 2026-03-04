/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <cstdint>

using CoordinatorType = int8_t;
inline constexpr CoordinatorType CoordinatorGroup = 0;
inline constexpr CoordinatorType CoordinatorTransaction = 1;