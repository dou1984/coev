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
	class co_event final : public queue
	{
		std::coroutine_handle<> m_caller = nullptr;
		// int m_status = CORO_INIT;
		std::atomic_int m_status = {CORO_INIT};
		uint64_t m_tid;
		uint64_t m_reserved;
		void __resume();
		friend void __ev_set_reserved(co_event &, uint64_t x);
		friend uint64_t __ev_get_reserved(co_event &);

	public:
		co_event(queue *_eventchain);
		virtual ~co_event();
		co_event(co_event &&) = delete;
		co_event(const co_event &) = delete;
		void await_resume();
		bool await_ready();
		void await_suspend(std::coroutine_handle<> awaitable);
		void resume();
		auto id() const { return m_tid; }
	};
}
