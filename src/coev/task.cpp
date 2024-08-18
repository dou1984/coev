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
	void task::insert(tasknotify* _notify)
	{		
		std::lock_guard<std::mutex> _(m_async.m_mutex);
		m_async.push_back(_notify);
		_notify->m_task = this;		
	}
	void task::__erase(tasknotify* _inotify)
	{
		LOG_CORE("erase %p\n", _inotify);
		std::lock_guard<std::mutex> _(m_async.m_mutex);
		m_async.erase(_inotify);
	}
	void task::destroy()
	{
		std::lock_guard<std::mutex> _(m_async.m_mutex);
		while (!m_async.empty())
		{
			auto t = static_cast<tasknotify *>(m_async.pop_front());			
			LOG_CORE("inotify:%p\n", t);
			m_async.erase(t);
			t->destroy();
		}
	}
	bool task::empty()
	{
		std::lock_guard<std::mutex> _(m_async.m_mutex);
		return m_async.empty();
	}
	void task::notify(tasknotify* _notify)
	{
		__erase(_notify);
		coev::notify(m_trigger);
	}
	event task::wait_for()
	{
		return coev::wait_for(m_trigger);
	}
}