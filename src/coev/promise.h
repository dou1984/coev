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
#include "queue.h"

namespace coev
{

	enum status : int
	{
		STATUS_INIT,
		STATUS_SUSPEND,
		STATUS_RESUMED,
	};

	class co_task;
	struct promise : queue
	{
		std::coroutine_handle<> m_caller = nullptr;
		co_task *m_task = nullptr;
		promise() = default;
		~promise();
		void unhandled_exception();
		std::suspend_never initial_suspend();
		std::suspend_never final_suspend() noexcept;
	};
	struct promise_void : promise
	{
		std::suspend_never return_void();
		std::suspend_always yield_void() = delete;
	};
	template <class V>
	struct promise_value : promise
	{
		V value;
		std::suspend_never return_value(V &&v)
		{
			value = std::forward<V>(v);
			return {};
		}
		std::suspend_never return_value(const V &v)
		{
			value = v;
			return {};
		}
		std::suspend_always yield_value(V &&v) = delete;

		std::suspend_always yield_value(const V &v) = delete;
	};
	
}
