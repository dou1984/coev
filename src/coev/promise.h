/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <coroutine>
#include <iostream>
#include <string.h>
#include "log.h"

namespace coev
{
	enum status : int
	{
		STATUS_INIT,
		STATUS_SUSPEND,
		STATUS_READY,
	};
	struct promise
	{
		promise() = default;
		~promise() = default;
		void unhandled_exception();
		std::suspend_never initial_suspend();
		std::suspend_never final_suspend() noexcept;
	};
}
