/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "event.h"
#include "task.h"

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
			auto c = static_cast<taskext *>(EVTask::pop_front());
			c->m_taskchain = nullptr;
			c->destroy();
		}
	}
	task::operator bool()
	{
		return !EVTask::empty();
	}
}