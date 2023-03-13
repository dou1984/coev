/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "Event.h"
#include "Task.h"

namespace coev
{
	void TaskExt::resume_ex()
	{
		if (m_TaskSet)
		{
			TRACE();
			m_TaskSet->EVTask::erase(this);
			m_TaskSet->EVEvent::resume_ex();
			m_TaskSet = nullptr;
		}
	}
	TaskExt::~TaskExt()
	{
		resume_ex();
	}
	void TaskSet::insert_task(Task *_task)
	{			
		EVTask::push_back(_task);
		_task->m_TaskSet = this;
	}
	void TaskSet::destroy()
	{
		while (!EVTask::empty())
		{
			TRACE();
			auto c = static_cast<Task *>(EVTask::pop_front());
			c->TaskExt::m_TaskSet = nullptr;
			c->destroy();
		}
	}
	TaskSet::operator bool()
	{
		return !EVTask::empty();
	}
}