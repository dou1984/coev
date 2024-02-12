/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "awaiter.h"

namespace coev
{
	awaiter::promise_type::~promise_type()
	{
		LOG_CORE("promise_type:%p awaiter:%p\n", this, m_awaiter);
		if (m_awaiter)
		{
			m_awaiter->m_state = READY;
			m_awaiter->resume();
			m_awaiter = nullptr;
		}
	}
	awaiter awaiter::promise_type::get_return_object()
	{
		return {std::coroutine_handle<promise_type>::from_promise(*this)};
	}
	std::suspend_never awaiter::promise_type::return_value(int v)
	{
		value = v;
		return {};
	}
	std::suspend_never awaiter::promise_type::yield_value(int v)
	{
		value = v;
		return {};
	}
	awaiter::awaiter(std::coroutine_handle<promise_type> h) : m_callee(h)
	{
		m_callee.promise().m_awaiter = this;
		m_state = INIT;
	}
	awaiter::~awaiter()
	{
		LOG_CORE("m_caller:%p m_callee:%p\n",
				 m_caller ? m_caller.address() : 0,
				 m_callee ? m_callee.address() : 0);
		if (m_callee.address())
			m_callee.promise().m_awaiter = nullptr;
	}
	void awaiter::resume()
	{
		m_state = READY;
		LOG_CORE("m_caller:%p m_callee:%p\n",
				 m_caller ? m_caller.address() : 0,
				 m_callee ? m_callee.address() : 0);
		if (m_caller.address() && !m_caller.done())
			m_caller.resume();
		taskevent::__resume();
	}
	bool awaiter::done()
	{
		return m_callee ? m_callee.done() : true;
	}
	bool awaiter::await_ready()
	{
		return m_state == READY;
	}
	void awaiter::await_suspend(std::coroutine_handle<> caller)
	{
		m_caller = caller;
		m_state = SUSPEND;
	}
	void awaiter::destroy()
	{
		LOG_CORE("m_caller:%p m_callee:%p\n",
				 m_caller ? m_caller.address() : 0,
				 m_callee ? m_callee.address() : 0);
		if (m_callee.address() && m_callee.promise().m_awaiter)
		{
			m_callee.promise().m_awaiter = nullptr;
			m_callee.destroy();
		}
		m_callee = nullptr;
		m_caller = nullptr;
	}
}