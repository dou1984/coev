/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <thread>
#include "co_event.h"
#include "local_resume.h"

namespace coev
{
	co_event::co_event(queue *_ev_queue) : m_tid(gtid())
	{
		if (_ev_queue != nullptr)
		{
			_ev_queue->push_back(this);
		}
	}
	co_event::~co_event()
	{
		if (!queue::empty())
		{
			queue::erase(this);
		}
		m_caller = nullptr;
	}
	void co_event::await_resume()
	{
	}
	bool co_event::await_ready()
	{
		assert(m_status == CORO_INIT || m_status == CORO_RESUMED);
		return m_status == CORO_RESUMED;
	}
	void co_event::await_suspend(std::coroutine_handle<> _awaitable)
	{
		local_resume();
		m_caller = _awaitable;
		if (m_status == CORO_INIT)
		{
			m_status = CORO_SUSPEND;
		}
		else if (m_status == CORO_RESUMED)
		{
			m_status = CORO_FINISHED;
			__resume();
		}
		else
		{
			assert(false);
		}
	}
	void co_event::resume()
	{
		LOG_CORE("co_event resume m_caller:%p\n", m_caller ? m_caller.address() : 0);
		if (m_status == CORO_INIT)
		{
			m_status = CORO_RESUMED;
		}
		else if (m_status == CORO_SUSPEND)
		{
			m_status = CORO_FINISHED;
			__resume();
		}
		else
		{
			assert(false);
		}
	}
	void co_event::__resume()
	{
		if (m_caller.address() && !m_caller.done())
		{
			auto _caller = m_caller;
			m_caller = nullptr;
			_caller.resume();
		}
	}
}
