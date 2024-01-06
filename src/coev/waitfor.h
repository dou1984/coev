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
	template <class EVCHAIN, class OBJ>
	event wait_for(OBJ &obj)
	{
		EVCHAIN *ev = &obj;
		return event{ev};
	}
	template <class EVCHAIN, class OBJ>
	event wait_for_x(OBJ &obj)
	{
		EVCHAIN *ev = &obj;
		return event{ev};
	}
	template <class EVCHAIN, class OBJ>
	bool resume(OBJ &obj)
	{
		EVCHAIN *ev = &obj;
		auto c = static_cast<event *>(ev->pop_front());
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
		co_await wait_for<EVEvent>(w);
		w.destroy();
		co_return 0;
	}
	template <class... T>
	awaiter wait_for_all(T &&..._task)
	{
		task w;
		(w.insert_task(&_task), ...);
		while (!w.empty())
			co_await wait_for<EVEvent>(w);
		co_return 0;
	}

}