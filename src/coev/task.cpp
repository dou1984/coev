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
	void task::insert(evtask *ev, int _id)
	{
		std::lock_guard<std::mutex> _(m_waiter.m_mutex);
		m_waiter.push_back(ev);
		ev->m_task = this;
		ev->m_id = _id;
	}
	void task::__erase(evtask *_inotify)
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
			auto t = static_cast<evtask *>(m_waiter.pop_front());
			LOG_CORE("evtask:%p\n", t);
			m_waiter.erase(t);
			t->destroy();
		}
	}
	bool task::empty()
	{
		std::lock_guard<std::mutex> _(m_waiter.m_mutex);
		return m_waiter.empty();
	}
	void task::done(evtask *ev)
	{
		m_listener.resume(
			[this, ev]()
			{
				m_last = ev->m_id;
				__erase(ev);
			});
	}
	awaitable<int> task::wait()
	{
		co_await m_listener.suspend(
			[]() -> bool
			{ return true; },
			[]() {});
		co_return m_last;
	}
}