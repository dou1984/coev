/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "event.h"
#include "gtid.h"

namespace coev
{
	event::event(chain *_eventchain, uint64_t _tid) : m_tid(_tid)
	{
		if (_eventchain != nullptr)
			_eventchain->push_back(this);
	}
	event::event(chain *_eventchain) : event(_eventchain, gtid())
	{
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
		return m_ready;
	}
	void event::await_suspend(std::coroutine_handle<> awaiting)
	{
		m_awaiting = awaiting;
	}
	void event::resume()
	{
		LOG_CORE("event m_awaiting:%p\n", m_awaiting ? m_awaiting.address() : 0);
		if (m_awaiting && !m_awaiting.done())
		{
			m_awaiting.resume();
		}
		m_ready = true;
	}
}