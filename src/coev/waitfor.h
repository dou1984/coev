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

	template <size_t I = 0, async_t... T>
	event wait_for(async<T...> *_this)
	{
		auto &_evl = std::get<I>(*_this);
		return event(&_evl);
	}
	template <size_t I = 0, async_t... T>
	bool resume(async<T...> *_this)
	{
		auto &_evl = std::get<I>(*_this);
		auto c = static_cast<event *>(_evl.pop_front());
		if (c)
		{
			c->resume();
			return true;
		}
		return false;
	}
	template <size_t I = 0, class SUSPEND, class CALL, async_t... T>
	awaiter wait_for(async<T...> *_this, const SUSPEND &suppend, const CALL &call)
	{
		auto &_evlts = std::get<I>(*_this);
		_evlts.lock();
		if (suppend())
		{
			event ev(&_evlts);
			_evlts.unlock();
			co_await ev;
			_evlts.lock();
		}
		call();
		_evlts.unlock();
		co_return 0;
	}
	template <size_t I = 0, class CALL, async_t... T>
	bool resume(async<T...> *_this, const CALL &call)
	{
		auto &_evlts = std::get<I>(*_this);
		_evlts.lock();
		auto c = static_cast<event *>(_evlts.pop_front());
		call();
		_evlts.unlock();
		if (c)
		{
			c->resume();
			return true;
		}
		return false;
	}
	template <class... AWAITER>
	awaiter wait_for_any(AWAITER &&...awt)
	{
		task w;
		(w.insert_task(&awt), ...);
		co_await wait_for<0>(&w);
		w.destroy();
		co_return 0;
	}
	template <class... AWAITER>
	awaiter wait_for_all(AWAITER &&...awt)
	{
		task w;
		(w.insert_task(&awt), ...);
		while (!w.empty())
		{
			co_await wait_for<0>(&w);
		}
		co_return 0;
	}

}