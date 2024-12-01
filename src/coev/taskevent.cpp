/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "taskevent.h"
#include "task.h"
#include "waitfor.h"
namespace coev
{
	void taskevent::resume()
	{
		if (m_task)
		{
			LOG_CORE("m_task %p\n", m_task);
			auto _task = m_task;
			m_task = nullptr;
			_task->done(this);
		}
	}
	taskevent::~taskevent()
	{
		resume();
	}
	void taskevent::destroy()
	{
	}
}