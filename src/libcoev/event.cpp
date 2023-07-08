/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "event.h"
#include "loop.h"

namespace coev
{
	event::event(chain *obj, uint32_t _tag) : m_eventchain(obj), m_tag(_tag)
	{
		if (m_eventchain != nullptr)
			m_eventchain->push_back(this);
	}
	event::event(chain *obj) : event(obj, loop::tag())
	{
	}
	event::~event()
	{
		if (m_eventchain != nullptr)
			m_eventchain->erase(this);
		m_eventchain = nullptr;
		m_awaiting = nullptr;
	}
	void event::await_resume()
	{
	}
	bool event::await_ready()
	{
		return m_eventchain == nullptr;
	}
	void event::await_suspend(std::coroutine_handle<> awaiting)
	{
		m_awaiting = awaiting;
	}
	void event::resume()
	{
		if (m_awaiting && !m_awaiting.done())
			m_awaiting.resume();
	}
}