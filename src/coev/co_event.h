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
	namespace guard
	{
		class async;
	}
	class co_event final : public queue
	{
		std::coroutine_handle<> m_caller = nullptr;
		std::atomic_int m_status = {CORO_INIT};
		uint64_t m_tid;
		uint64_t m_reserved;
		void __resume();
		void __set_reserved(uint64_t x);
		uint64_t __get_reserved();

		friend class coev::guard::async;

	public:
		co_event(queue *__ev, bool __seq);
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
