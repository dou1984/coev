/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "Event.h"
#include "Loop.h"

namespace coev
{
	Event::Event(Chain *obj, uint32_t _tag) : m_object(obj), m_tag(_tag)
	{
		if (m_object != nullptr)
			m_object->push_back(this);
	}
	Event::Event(Chain *obj) : Event(obj, Loop::tag())
	{
	}
	Event::~Event()
	{
		if (m_object != nullptr)
			m_object->erase(this);
		m_object = nullptr;
		m_awaiting = nullptr;
	}
	void Event::await_resume()
	{
	}
	bool Event::await_ready()
	{
		return m_object == nullptr;
	}
	void Event::await_suspend(std::coroutine_handle<> awaiting)
	{
		m_awaiting = awaiting;
	}
	void Event::resume_ex()
	{
		if (m_awaiting && !m_awaiting.done())
			m_awaiting.resume();
	}
}