/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "event.h"
#include "eventchain.h"

namespace coev
{
	
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
		m_ready = true;
		if (m_awaiting && !m_awaiting.done())
		{
			m_awaiting.resume();
		}
	}
}