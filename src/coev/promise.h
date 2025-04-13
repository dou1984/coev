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
#include "gtid.h"
#include "log.h"
#include "queue.h"
#include "suspend_bool.h"

namespace coev
{

	enum status : int
	{
		CORO_INIT,
		CORO_SUSPEND,
		CORO_RUNNING,
		CORO_RESUMED,
		CORO_FINISHED,

	};

	class co_task;
	struct promise : queue
	{
		std::coroutine_handle<> m_this = nullptr;	
		std::coroutine_handle<> m_caller = nullptr;		
		co_task *m_task = nullptr;
		int m_status = CORO_INIT;
		uint64_t m_tid = gtid();
		promise() = default;
		~promise();
		void unhandled_exception();
		suspend_bool initial_suspend();
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
			LOG_CORE("return_value %p\n", this);
			value = std::forward<V>(v);
			return {};
		}
		std::suspend_never return_value(const V &v)
		{
			LOG_CORE("return_value %p\n", this);
			value = v;
			return {};
		}
		std::suspend_always yield_value(V &&v) = delete;
		std::suspend_always yield_value(const V &v) = delete;
	};

}
