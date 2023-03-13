/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "Promise.h"

namespace coev
{
	void Promise::unhandled_exception()
	{
		throw std::current_exception();
	}
	std::suspend_never Promise::initial_suspend()
	{
		return {};
	}
	std::suspend_never Promise::final_suspend() noexcept
	{
		return {};
	}
}