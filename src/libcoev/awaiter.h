/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <coroutine>
#include "chain.h"
#include "Promise.h"
#include "log.h"
#include "object.h"
#include "hook.h"
#include "taskext.h"

namespace coev
{

	template <class Ret = int>
	struct awaiter final : taskext
	{
		struct promise_type : promise
		{
			Ret value;
			awaiter *_this = nullptr;
			promise_type() = default;
			~promise_type()
			{
				if (_this != nullptr)
				{
					_this->m_ready = true;
					_this->resume();
				}
			}
			awaiter get_return_object()
			{
				return {std::coroutine_handle<promise_type>::from_promise(*this)};
			}
			std::suspend_never return_value(Ret &&v)
			{
				value = std::move(v);
				return {};
			}
			std::suspend_never yield_value(Ret &&v)
			{
				value = std::move(v);
				return {};
			}
		};
		awaiter() = default;
		awaiter(std::coroutine_handle<promise_type> h) : m_coroutine(h)
		{
			m_coroutine.promise()._this = this;
		}
		awaiter(awaiter &&o) = delete;
		awaiter(const awaiter &) = delete;
		const awaiter &operator=(awaiter &&) = delete;
		~awaiter()
		{
			if (m_coroutine != nullptr)
				m_coroutine.promise()._this = nullptr;
		}
		void resume()
		{
			if (m_awaiting && !m_awaiting.done())
				m_awaiting.resume();
			taskext::__resume();
		}
		bool done() { return m_coroutine ? m_coroutine.done() : true; }
		bool await_ready() { return m_ready; }
		void await_suspend(std::coroutine_handle<> awaiting) { m_awaiting = awaiting; }
		auto await_resume() { return m_coroutine ? m_coroutine.promise().value : Ret{}; }
		void destroy()
		{
			if (m_coroutine)
			{
				m_coroutine.promise()._this = nullptr;
				m_coroutine.destroy();
				m_coroutine = nullptr;
				m_awaiting = nullptr;
			}
		}
		std::coroutine_handle<promise_type> m_coroutine = nullptr;
		std::coroutine_handle<> m_awaiting = nullptr;
		bool m_ready = false;
	};
}