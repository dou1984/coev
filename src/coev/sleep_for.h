/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include "awaitable.h"

namespace coev
{
	awaitable<void> sleep_for(double);
	awaitable<void> usleep_for(double);
}