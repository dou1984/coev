/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
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
		std::atomic_int m_status = {details::CORO_INIT};
		uint64_t m_tid;
		int64_t m_reserved;
		void __resume();

	public:
		co_event(queue *__ev);
		virtual ~co_event();
		co_event(co_event &&) = delete;
		co_event(const co_event &) = delete;
		int64_t await_resume();
		bool await_ready();
		void await_suspend(std::coroutine_handle<> caller);
		void resume();
		auto id() const { return m_tid; }

	public:
		void __set_reserved(int64_t x);
		int64_t __get_reserved();
	};
}
