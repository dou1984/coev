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
	void task::__insert(taskevent *_task)
	{
		std::lock_guard<std::mutex> _(m_async);
		m_async.push_back(_task);
		_task->m_taskchain = this;
	}
	void task::__erase(taskevent *_task)
	{
		std::lock_guard<std::mutex> _(m_async);
		_task->m_taskchain = nullptr;
		m_async.erase(_task);
	}
	void task::destroy()
	{
		std::lock_guard<std::mutex> _(m_async);
		while (!m_async.empty())
		{
			auto t = static_cast<taskevent *>(m_async.pop_front());
			assert(t->m_taskchain);
			LOG_CORE("t:%p taskchain:%p\n", t, t->m_taskchain);
			t->m_taskchain = nullptr;			
			t->destroy();
		}
	}
	bool task::empty()
	{
		std::lock_guard<std::mutex> _(m_async);
		return m_async.empty();
	}
}