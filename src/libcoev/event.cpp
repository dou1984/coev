/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "event.h"
#include "Loop.h"

namespace coev
{
	event::event(chain *obj, uint32_t _tag) : m_object(obj), m_tag(_tag)
	{
		if (m_object != nullptr)
			m_object->push_back(this);
	}
	event::event(chain *obj) : event(obj, Loop::tag())
	{
	}
	event::~event()
	{
		if (m_object != nullptr)
			m_object->erase(this);
		m_object = nullptr;
		m_awaiting = nullptr;
	}
	void event::await_resume()
	{
	}
	bool event::await_ready()
	{
		return m_object == nullptr;
	}
	void event::await_suspend(std::coroutine_handle<> awaiting)
	{
		m_awaiting = awaiting;
	}
	void event::resume_ex()
	{
		if (m_awaiting && !m_awaiting.done())
			m_awaiting.resume();
	}
}