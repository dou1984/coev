/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <thread>
#include "event.h"
#include "eventchain.h"
#include "gtid.h"

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
		m_awaiting = nullptr;
	}
	void event::await_resume()
	{
	}
	bool event::await_ready()
	{
		return m_status == READY;
	}
	void event::await_suspend(std::coroutine_handle<> awaiting)
	{
		m_awaiting = awaiting;
		m_status = SUSPEND;
	}
	void event::resume()
	{
		LOG_CORE("event m_awaiting:%p\n", m_awaiting ? m_awaiting.address() : 0);
		while (m_status == CONSTRUCT)
		{
			std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		}
		m_status = READY;
		if (m_awaiting.address() && !m_awaiting.done())
		{
			m_awaiting.resume();
		}
	}
}