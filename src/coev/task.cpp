/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "event.h"
#include "task.h"
#include "awaitable.h"
#include "waitfor.h"

namespace coev
{
	task::~task()
	{
		destroy();
	}
	void task::insert(taskevent *ev)
	{
		std::lock_guard<std::mutex> _(m_waiter.m_mutex);
		m_waiter.push_back(ev);
		ev->m_task = this;
	}
	void task::__erase(taskevent *_inotify)
	{
		LOG_CORE("erase %p\n", _inotify);
		std::lock_guard<std::mutex> _(m_waiter.m_mutex);
		m_waiter.erase(_inotify);
	}
	void task::destroy()
	{
		std::lock_guard<std::mutex> _(m_waiter.m_mutex);
		while (!m_waiter.empty())
		{
			auto t = static_cast<taskevent *>(m_waiter.pop_front());
			LOG_CORE("inotify:%p\n", t);
			m_waiter.erase(t);
			t->destroy();
		}
	}
	bool task::empty()
	{
		std::lock_guard<std::mutex> _(m_waiter.m_mutex);
		return m_waiter.empty();
	}
	void task::done(taskevent *ev)
	{
		__erase(ev);
		m_listener.resume();
	}
	event task::wait()
	{
		return m_listener.suspend();
	}
}