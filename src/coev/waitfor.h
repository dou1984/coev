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

namespace coev
{
	template <class ASYNC>
	event wait_for(ASYNC *_this)
	{
		return event(_this);
	}
	template <class ASYNC>
	bool resume(ASYNC *_this)
	{
		auto c = static_cast<event *>(_this->pop_front());
		if (c)
		{
			c->resume();
			return true;
		}
		return false;
	}
	template <class... T>
	awaiter wait_for_any(T &&..._task)
	{
		task w;
		(w.insert_task(&_task), ...);
		co_await wait_for<EVEvent>(&w);
		w.destroy();
		co_return 0;
	}
	template <class... T>
	awaiter wait_for_all(T &&..._task)
	{
		task w;
		(w.insert_task(&_task), ...);
		while (!w.empty())
			co_await wait_for<EVEvent>(&w);
		co_return 0;
	}
	namespace ts
	{
		template <class ASYNC, class SUSPEND, class CALL>
		awaiter wait_for(ASYNC *_this, const SUSPEND &suppend, const CALL &call)
		{
			return _this->wait_for(suppend, call);
		}
		template <class ASYNC, class CALL>
		bool resume(ASYNC *_this, const CALL &call)
		{
			return _this->resume(call);
		}
	}
}