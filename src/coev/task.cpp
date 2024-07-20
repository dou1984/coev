/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "event.h"
#include "task.h"
#include "awaiter.h"

namespace coev
{
	task::~task()
	{
		destroy();
	}
	void task::insert_task(taskevent *_task)
	{
		std::lock_guard<std::mutex> _(m_trigger_mutex);
		m_trigger_mutex.push_back(_task);
		_task->m_taskchain = this;
	}
	void task::erase_task(taskevent *_task)
	{
		std::lock_guard<std::mutex> _(m_trigger_mutex);
		_task->m_taskchain = nullptr;
		m_trigger_mutex.erase(_task);
	}
	void task::destroy()
	{
		std::lock_guard<std::mutex> _(m_trigger_mutex);
		while (!m_trigger_mutex.empty())
		{
			auto t = static_cast<taskevent *>(m_trigger_mutex.pop_front());
			assert(t->m_taskchain);
			LOG_CORE("t:%p taskchain:%p\n", t, t->m_taskchain);
			t->m_taskchain = nullptr;			
			t->destroy();
		}
	}
	bool task::empty()
	{
		std::lock_guard<std::mutex> _(m_trigger_mutex);
		return m_trigger_mutex.empty();
	}
}