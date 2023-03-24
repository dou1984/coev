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
#include "Hook.h"

namespace coev
{

	template <class Ret = int, class Extend = AWAITER>
	struct Awaiter : Extend
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
		Awaiter(std::coroutine_handle<promise_type> h) : m_coroutine(h)
		{
			m_coroutine.promise()._this = this;
		}
		Awaiter(Awaiter &&o) = delete;
		Awaiter(const Awaiter &) = delete;
		const Awaiter &operator=(Awaiter &&) = delete;
		~Awaiter()
		{
			if (m_coroutine != nullptr)
				m_coroutine.promise()._this = nullptr;
		}
		void resume()
		{
			if (m_coroutine && !m_coroutine.done())
				m_coroutine.resume();
		}
		void resume_ex()
		{
			if (m_awaiting && !m_awaiting.done())
				m_awaiting.resume();
			Extend::resume_ex();
		}
		bool done()
		{
			return m_coroutine ? m_coroutine.done() : true;
		}
		bool await_ready()
		{
			return m_ready;
		}
		void await_suspend(std::coroutine_handle<> awaiting)
		{
			m_awaiting = awaiting;
		}
		auto await_resume()
		{
			return m_coroutine ? m_coroutine.promise().value : Ret{};
		}
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