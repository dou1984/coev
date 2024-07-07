/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "promise.h"
#include "log.h"
#include "awaiter.h"

namespace coev
{
	void promise::unhandled_exception()
	{
		throw std::current_exception();
	}
	std::suspend_never promise::initial_suspend()
	{
		return {};
	}
	std::suspend_never promise::final_suspend() noexcept
	{
		return {};
	}
}