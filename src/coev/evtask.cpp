/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "evtask.h"
#include "co_task.h"
#include "wait_for.h"
namespace coev
{
	evtask::evtask(co_task *_task, int _id) : m_task(_task), m_id(_id)
	{
	}
	void evtask::resume()
	{
		if (m_task)
		{
			LOG_CORE("m_task %p\n", m_task);
			auto _task = m_task;
			m_task = nullptr;
			_task->done(this);
		}
	}
	evtask::~evtask()
	{
		resume();
	}
	void evtask::destroy()
	{
	}
}