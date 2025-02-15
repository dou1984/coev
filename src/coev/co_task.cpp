/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "event.h"
#include "co_task.h"
#include "awaitable.h"
#include "waitfor.h"

namespace coev
{
	co_task::~co_task()
	{
		destroy();
	}
	void co_task::insert(evtask *ev, int _id)
	{
		std::lock_guard<std::mutex> _(m_ev_waiter.m_mutex);
		m_ev_waiter.push_back(ev);
		ev->m_task = this;
		ev->m_id = _id;
	}
	void co_task::__erase(evtask *_inotify)
	{
		LOG_CORE("erase %p\n", _inotify);
		std::lock_guard<std::mutex> _(m_ev_waiter.m_mutex);
		m_ev_waiter.erase(_inotify);
	}
	void co_task::destroy()
	{
		std::lock_guard<std::mutex> _(m_ev_waiter.m_mutex);
		while (!m_ev_waiter.empty())
		{
			auto t = static_cast<evtask *>(m_ev_waiter.pop_front());
			LOG_CORE("evtask:%p\n", t);
			m_ev_waiter.erase(t);
			t->destroy();
		}
	}
	bool co_task::empty()
	{
		std::lock_guard<std::mutex> _(m_ev_waiter.m_mutex);
		return m_ev_waiter.empty();
	}
	void co_task::done(evtask *ev)
	{
		m_task_waiter.resume(
			[this, ev]()
			{
				m_last = ev->m_id;
				__erase(ev);
			});
	}
	awaitable<int> co_task::wait()
	{
		co_await m_task_waiter.suspend(
			[]() -> bool
			{ return true; },
			[]() {});
		co_return m_last;
	}
}