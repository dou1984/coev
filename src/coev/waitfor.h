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

	namespace ts
	{
		awaiter<int> wait_for(ts::async &, const SUSPEND &suppend, const CALL &call);
		bool resume(ts::async &, const CALL &call);
	}

}