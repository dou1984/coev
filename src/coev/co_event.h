/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <coroutine>
#include <atomic>
#include "log.h"
#include "queue.h"
#include "promise.h"
#include "gtid.h"

namespace coev
{
	struct co_event final : queue
	{
		int m_status = CORO_INIT;
		std::coroutine_handle<> m_caller = nullptr;
		size_t m_tid;
		co_event(queue *_eventchain);
		virtual ~co_event();
		co_event(co_event &&) = delete;
		co_event(const co_event &) = delete;
		void await_resume();
		bool await_ready();
		void await_suspend(std::coroutine_handle<> awaitable);
		void resume();
	};
}
