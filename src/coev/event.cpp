/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <thread>
#include "event.h"

namespace coev
{
	event::event(chain *_eventchain) : m_tid(gtid())
	{
		if (_eventchain != nullptr)
			_eventchain->push_back(this);
	}
	event::~event()
	{
		if (!chain::empty())
			chain::erase(this);
		m_awaiter = nullptr;
	}
	void event::await_resume()
	{
	}
	bool event::await_ready()
	{
		return m_status == STATUS_READY;
	}
	void event::await_suspend(std::coroutine_handle<> awaiter)
	{
		m_awaiter = awaiter;
		m_status = STATUS_SUSPEND;
	}
	void event::resume()
	{
		LOG_CORE("event m_awaiter:%p\n", m_awaiter ? m_awaiter.address() : 0);		
		while (m_status == STATUS_INIT)
		{
			std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		}		
		m_status = STATUS_READY;
		if (m_awaiter.address() && !m_awaiter.done())
		{
			m_awaiter.resume();
		}
	}
}