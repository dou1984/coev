/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <chrono>
#include <coev/coev.h>

coev::awaitable<void> sleep_for(std::chrono::milliseconds);
coev::awaitable<void> sleep_for(std::chrono::seconds);