/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <coroutine>
#include <iostream>
#include <string.h>
#include <atomic>
#include "gtid.h"
#include "log.h"
#include "queue.h"
#include "suspend_ready.h"

namespace coev
{

	namespace details
	{
		enum status : int16_t
		{
			CORO_INIT,
			CORO_SUSPEND,
			CORO_RUNNING,
			CORO_RESUMED,
			CORO_FINISHED,

		};
		enum type : int16_t
		{
			CORO_NONE,
			CORO_TASK,
			CORO_GUARD_TASK,
			CORO_COROUTINE_HANDLE,

		};
	}
	struct promise : queue
	{
		std::coroutine_handle<> m_this = nullptr;
		union
		{
			std::coroutine_handle<> m_caller = nullptr;
			void *m_task;
		};
		uint64_t m_tid = gtid();
		int16_t m_status = details::CORO_INIT;
		int16_t m_type = details::CORO_NONE;
		promise();
		~promise();
		void unhandled_exception();
		suspend_ready initial_suspend();
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
