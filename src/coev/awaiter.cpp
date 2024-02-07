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
	awaiter::awaiter(std::coroutine_handle<promise_type> h) : m_coroutine(h)
	{
		m_coroutine.promise().m_awaiter = this;
		m_state = INIT;
	}
	awaiter::~awaiter()
	{
		LOG_CORE("m_awaiting:%p m_coroutine:%p\n",
				 m_awaiting ? m_awaiting.address() : 0,
				 m_coroutine ? m_coroutine.address() : 0);
		if (m_coroutine.address())
			m_coroutine.promise().m_awaiter = nullptr;
	}
	void awaiter::resume()
	{
		m_state = READY;
		LOG_CORE("m_awaiting:%p m_coroutine:%p\n",
				 m_awaiting ? m_awaiting.address() : 0,
				 m_coroutine ? m_coroutine.address() : 0);
		if (m_awaiting.address() && !m_awaiting.done())
			m_awaiting.resume();
		taskevent::__resume();
	}
	bool awaiter::done()
	{
		return m_coroutine ? m_coroutine.done() : true;
	}
	bool awaiter::await_ready()
	{
		return m_state == READY;
	}
	void awaiter::await_suspend(std::coroutine_handle<> awaiting)
	{
		m_awaiting = awaiting;
		m_state = SUSPEND;
	}
	void awaiter::destroy()
	{
		LOG_CORE("m_awaiting:%p m_coroutine:%p\n",
				 m_awaiting ? m_awaiting.address() : 0,
				 m_coroutine ? m_coroutine.address() : 0);
		if (m_coroutine.address() && m_coroutine.promise().m_awaiter)
		{
			m_coroutine.promise().m_awaiter = nullptr;
			m_coroutine.destroy();
		}
		m_coroutine = nullptr;
		m_awaiting = nullptr;
	}
}