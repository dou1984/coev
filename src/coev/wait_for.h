/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <memory>
#include <functional>
#include "awaitable.h"
#include "co_task.h"
#include "co_event.h"
#include "async.h"

namespace coev
{
	template <class... AWAITABLE>
	awaitable<int> wait_for_any(AWAITABLE &&...awt)
	{
		co_task w;
		((w << awt), ...);
		auto id = co_await w.wait();
		w.destroy();
		co_return id;
	}
	template <class... AWAITABLE>
	awaitable<int> wait_for_all(AWAITABLE &&...awt)
	{
		co_task w;
		((w << awt), ...);
		while (!w.empty())
		{
			co_await w.wait();
		}
		co_return 0;
	}
}