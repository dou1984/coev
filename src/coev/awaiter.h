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
	class awaiter final : public taskevent
	{
	public:
		struct promise_type : promise
		{
			T value;
			awaiter *m_awaiter = nullptr;
			promise_type() = default;
			~promise_type()
			{
				LOG_CORE("promise_type:%p awaiter:%p\n", this, m_awaiter);
				if (m_awaiter)
				{
					m_awaiter->m_state = STATUS_READY;
					m_awaiter->resume();
					m_awaiter = nullptr;
				}
			}
			awaiter<T> get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
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
		std::coroutine_handle<promise_type> m_callee = nullptr;
		std::coroutine_handle<> m_caller = nullptr;
		std::atomic_int m_state{STATUS_INIT};

	public:
		awaiter() = default;
		awaiter(std::coroutine_handle<promise_type> h) : m_callee(h)
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
		void await_suspend(std::coroutine_handle<> caller)
		{
			m_caller = caller;
			m_state = STATUS_SUSPEND;
		}
		bool await_ready()
		{
			return m_state == STATUS_READY;
		}
		void resume()
		{
			m_state = STATUS_READY;
			LOG_CORE("m_caller:%p\n", m_caller ? m_caller.address() : 0)
			if (m_caller.address() && !m_caller.done())
				m_caller.resume();
			taskevent::__resume();
		}
	};

}