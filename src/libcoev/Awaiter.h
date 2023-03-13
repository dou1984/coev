/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <coroutine>
#include "Chain.h"
#include "Promise.h"
#include "Log.h"
#include "Object.h"

namespace coev
{
	template <class Ret = int, class Base = AWAITER>
	struct Awaiter : Base
	{
		struct promise_type : Promise
		{
			Ret value;
			Awaiter *_this = nullptr;
			promise_type() = default;
			~promise_type()
			{
				if (_this != nullptr)
				{
					_this->m_ready = true;
					_this->resume_ex();
				}
			}
			Awaiter get_return_object()
			{
				return {std::coroutine_handle<promise_type>::from_promise(*this)};
			}
			std::suspend_never return_value(Ret &&v)
			{
				value = std::move(v);
				return {};
			}
		};
		Awaiter() = default;
		Awaiter(std::coroutine_handle<promise_type> h) : m_coro(h)
		{
			m_coro.promise()._this = this;
		}
		Awaiter(Awaiter &&o) = delete;
		Awaiter(const Awaiter &) = delete;
		const Awaiter &operator=(Awaiter &&) = delete;
		~Awaiter()
		{
			if (m_coro != nullptr)
				m_coro.promise()._this = nullptr;
		}
		void resume()
		{
			if (m_coro && !m_coro.done())
				m_coro.resume();
		}
		void resume_ex()
		{
			if (m_awaiting && !m_awaiting.done())
				m_awaiting.resume();
			Base::resume_ex();
		}
		bool done()
		{
			return m_coro ? m_coro.done() : true;
		}
		bool await_ready()
		{
			TRACE();
			return m_ready;
		}
		void await_suspend(std::coroutine_handle<> awaiting)
		{
			TRACE();
			m_awaiting = awaiting;
		}
		auto await_resume()
		{
			return m_coro ? m_coro.promise().value : Ret{};
		}
		void destroy()
		{
			if (m_coro)
			{
				TRACE();
				m_coro.promise()._this = nullptr;
				m_coro.destroy();
				m_coro = nullptr;
				m_awaiting = nullptr;
			}
		}
		std::coroutine_handle<promise_type> m_coro = nullptr;
		std::coroutine_handle<> m_awaiting = nullptr;
		bool m_ready = false;
	};
}