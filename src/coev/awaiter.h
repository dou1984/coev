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
#include "chain.h"
#include "promise.h"
#include "log.h"
#include "taskevent.h"

namespace coev
{
	template <class T>
	class awaiter;
	namespace details
	{
		struct awaiter_impl : taskevent
		{
			std::coroutine_handle<> m_caller = nullptr;
			std::atomic_int m_state{STATUS_INIT};
			void await_suspend(std::coroutine_handle<> caller);
			bool await_ready();
			void resume();
		};
		template <class T>
		struct promise_type : promise
		{
			T value;
			awaiter<T> get_return_object() { return {std::coroutine_handle<promise_type<T>>::from_promise(*this)}; }
			std::suspend_never return_value(T &&v)
			{
				value = std::move(v);
				return {};
			}
			std::suspend_never yield_value(T &&v)
			{
				value = std::move(v);
				return {};
			}
		};
		struct promise_void : promise
		{
			//awaiter<void> get_return_object() { return {std::coroutine_handle<promise_void>::from_promise(*this)}; }
			std::suspend_never return_void() { return {}; }
			std::suspend_never yield_void() { return {}; }
		};
	}
	template <class T>
	class awaiter final : protected details::awaiter_impl
	{
		using PROMISE = std::conditional<std::is_void_v<T>, details::promise_void, details::promise_type<T>>;
		std::coroutine_handle<PROMISE> m_callee = nullptr;

	public:
		awaiter() = default;
		awaiter(std::coroutine_handle<PROMISE> h) : m_callee(h)
		{
			m_callee.promise().m_awaiter = this;
		}
		awaiter(awaiter &&o) = delete;
		awaiter(const awaiter &) = delete;
		const awaiter &operator=(awaiter &&) = delete;
		~awaiter()
		{
			LOG_CORE("m_caller:%p m_callee:%p\n", m_caller ? m_caller.address() : 0, m_callee ? m_callee.address() : 0);
			if (m_callee.address())
				m_callee.promise().m_awaiter = nullptr;
		}
		bool done() { return m_callee ? m_callee.done() : true; }
		auto await_resume() { return m_callee ? m_callee.promise().value : 0; }
		void destroy()
		{
			LOG_CORE("m_caller:%p m_callee:%p\n", m_caller ? m_caller.address() : 0, m_callee ? m_callee.address() : 0);
			if (m_callee.address() && m_callee.promise().m_awaiter)
			{
				m_callee.promise().m_awaiter = nullptr;
				m_callee.destroy();
			}
			m_callee = nullptr;
			m_caller = nullptr;
		}
	};

}