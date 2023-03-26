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
	void taskext::resume_ex()
	{
		if (m_TaskSet)
		{
			TRACE();
			m_TaskSet->EVTask::erase(this);
			m_TaskSet->EVEvent::resume_ex();
			m_TaskSet = nullptr;
		}
	}
	taskext::~taskext()
	{
		resume_ex();
	}
	void taskchain::insert_task(taskext *_task)
	{
		EVTask::push_back(_task);
		_task->m_TaskSet = this;
		TRACE();
	}
	void taskchain::destroy()
	{
		while (!EVTask::empty())
		{
			TRACE();
			auto c = static_cast<task *>(EVTask::pop_front());
			c->taskext::m_TaskSet = nullptr;
			c->destroy();
		}
	}
	taskchain::operator bool()
	{
		return !EVTask::empty();
	}
}