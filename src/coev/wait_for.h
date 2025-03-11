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
#include "co_task.h"
#include "event.h"
#include "async.h"

namespace coev
{
	template <class... AWAITABLE>
	awaitable<int> wait_for_any(AWAITABLE &&...awt)
	{
		co_task w;
		int id = 0;
		(w.insert(id++, &awt), ...);
		id = co_await w.wait();
		w.destroy();
		co_return id;
	}
	template <class... AWAITABLE>
	awaitable<int> wait_for_all(AWAITABLE &&...awt)
	{
		co_task w;
		int id = 0;
		(w.insert(id++, &awt), ...);
		while (!w.empty())
		{
			id = co_await w.wait();
		}
		co_return id;
	}
}