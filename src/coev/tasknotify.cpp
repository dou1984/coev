/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "tasknotify.h"
#include "task.h"
#include "waitfor.h"
namespace coev
{
	void tasknotify::notify()
	{
		if (m_task)
		{
			LOG_CORE("m_task %p\n", m_task);
			auto _task = m_task;
			m_task = nullptr;
			_task->notify(this);
		}
	}
	tasknotify::~tasknotify()
	{
		notify();
	}
	void tasknotify::destroy()
	{
	}
}