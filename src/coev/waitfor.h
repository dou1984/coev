/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <memory>
#include <functional>
#include "awaitable.h"
#include "task.h"
#include "event.h"
#include "async.h"

namespace coev
{
	template <class... AWAITABLE>
	awaitable<int> wait_for_any(AWAITABLE &&...awt)
	{
		task w;
		(w.insert(&awt), ...);
		co_await w.wait();
		w.destroy();
		co_return 0;
	}
	template <class... AWAITABLE>
	awaitable<int> wait_for_all(AWAITABLE &&...awt)
	{
		task w;
		(w.insert(&awt), ...);
		while (!w.empty())
		{
			co_await w.wait();
		}
		co_return 0;
	}
}