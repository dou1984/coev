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

	void task::insert_task(taskext *_task)
	{
		EVTask::push_back(_task);
		_task->m_taskchain = this;
	}
	void task::destroy()
	{
		while (!EVTask::empty())
		{
			auto t = static_cast<taskext *>(EVTask::pop_front());
			assert(t->m_taskchain);
			LOG_CORE("t:%p taskchain:%p\n", t, t->m_taskchain);
			t->m_taskchain = nullptr;
			auto a = static_cast<awaiter *>(t);
			a->destroy();
		}
	}
	task::operator bool()
	{
		return !EVTask::empty();
	}
}