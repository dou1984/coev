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
#include <memory>
#include <iostream>
#include <tuple>
#include "queue.h"
#include "promise.h"
#include "log.h"
#include "local.h"
#include "is_destroying.h"

namespace coev
{
	template <class T>
	class awaitable final
	{
	public:
		struct promise_type : std::conditional_t<std::is_void_v<T>, promise_void, promise_value<T>>
		{
			awaitable<T> get_return_object()
			{
				return {std::coroutine_handle<promise_type>::from_promise(*this)};
			}
		};

	public:
		awaitable() =default;
		awaitable(std::coroutine_handle<promise_type> h) : m_callee(h)
		{
			m_callee.promise().m_this = m_callee;
		}
		awaitable(awaitable &&o) = delete;
		awaitable(const awaitable &) = delete;
		const awaitable &operator=(awaitable &&) = delete;
		const awaitable &operator=(const awaitable &&) = delete;
		~awaitable()
		{
			if (local<is_destroying>::instance())
			{
				destroy();
			}
		}
		bool done()
		{
			return m_callee && m_callee.address() && (m_callee.done() || m_callee.promise().m_status == CORO_FINISHED);
		}
		auto await_resume() // 返回协程的返回值
		{
			if constexpr (!std::is_void<T>::value)
			{
				return m_callee && m_callee.address() != nullptr ? std::move(m_callee.promise().value) : decltype(m_callee.promise().value){};
			}
		}
		void destroy()
		{
			if (!done())
			{
				auto &_promise = m_callee.promise();
				_promise.m_task = nullptr;
				_promise.m_caller = nullptr;
				std::lock_guard<is_destroying> _(local<is_destroying>::instance());
				m_callee.destroy();
			}
		}
		void await_suspend(std::coroutine_handle<> caller) // co_await调用， 传入上层coroutine_handle
		{
			m_callee.promise().m_caller = caller;
			if (m_callee.promise().m_status == CORO_SUSPEND)
			{
				// m_callee should be resumed here
				m_callee.resume();
			}
			else if (m_callee.promise().m_status == CORO_INIT)
			{
			}
			else
			{
				throw std::runtime_error("await_suspend error m_callee.promise().m_status");
			}
		}
		bool await_ready() // 是否挂起,正常情况需要挂起,返回false
		{
			return done();
		}
		operator promise_type *()
		{
			return m_callee ? (&m_callee.promise()) : nullptr;
		}

	private:
		std::coroutine_handle<promise_type> m_callee = nullptr;
	};
}