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
#include "awaiter.h"
#include "task.h"
#include "event.h"
#include "async.h"

namespace coev
{
	using SUSPEND = std::function<bool()>;
	using CALL = std::function<void()>;

	event wait_for(async &);
	bool resume(async &);

	awaiter<int> wait_for_ts(async_ts &, const SUSPEND &suppend, const CALL &call);
	bool resume_ts(async_ts &, const CALL &call);

	template <class... AWAITER>
	awaiter<int> wait_for_any(AWAITER &&...awt)
	{
		task w;
		(w.insert_task(&awt), ...);
		co_await wait_for(w.m_trigger);
		w.destroy();
		co_return 0;
	}
	template <class... AWAITER>
	awaiter<int> wait_for_all(AWAITER &&...awt)
	{
		task w;
		(w.insert_task(&awt), ...);
		while (!w.empty())
		{
			co_await wait_for(w.m_trigger);
		}
		co_return 0;
	}
}