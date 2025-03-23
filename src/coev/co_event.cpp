/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <thread>
#include "co_event.h"

namespace coev
{
	co_event::co_event(queue *_eventchain) : m_tid(gtid())
	{
		if (_eventchain != nullptr)
		{
			_eventchain->push_back(this);
		}
		LOG_CORE("co_event _eventchain %p\n", _eventchain);
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
		return m_status == CORO_RESUMED;
	}
	void co_event::await_suspend(std::coroutine_handle<> _awaitable)
	{
		m_caller = _awaitable;
		m_status = CORO_SUSPEND;
	}
	void co_event::resume()
	{
		LOG_CORE("co_event resume m_caller:%p\n", m_caller ? m_caller.address() : 0);
		while (m_status == CORO_INIT)
		{
			std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		}
		m_status = CORO_RESUMED;
		if (m_caller.address() && !m_caller.done())
		{
			m_caller.resume();
		}
	}
}