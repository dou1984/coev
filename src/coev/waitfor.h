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
#include "evlist.h"

namespace coev
{
	template <class ASYNC = async>
	event wait_for(ASYNC *_this)
	{
		return event(_this);
	}
	template <class ASYNC = async>
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
		co_await wait_for<async_ext>(&w);
		w.destroy();
		co_return 0;
	}
	template <class... T>
	awaiter wait_for_all(T &&..._task)
	{
		task w;
		(w.insert_task(&_task), ...);
		while (!w.empty())
			co_await wait_for<async_ext>(&w);
		co_return 0;
	}
	namespace ts
	{
		template <class SUSPEND, class CALL>
		awaiter wait_for(async_mutex *_this, const SUSPEND &suppend, const CALL &call)
		{
			_this->lock();
			if (suppend())
			{
				event ev(_this);
				_this->unlock();
				co_await ev;
				_this->lock();
			}
			call();
			_this->unlock();
			co_return 0;
		}
		template <class CALL>
		bool resume(async_mutex *_this, const CALL &call)
		{
			_this->lock();
			auto c = static_cast<event *>(_this->pop_front());
			call();
			_this->unlock();
			if (c)
			{
				c->resume();
				return true;
			}
			return false;
		}
	}
}