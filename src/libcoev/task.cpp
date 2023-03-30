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
		if (m_taskchain)
		{
			TRACE();
			m_taskchain->EVTask::erase(this);
			m_taskchain->EVEvent::resume_ex();
			m_taskchain = nullptr;
		}
	}
	taskext::~taskext()
	{
		resume_ex();
	}
	void taskchain::insert_task(taskext *_task)
	{
		EVTask::push_back(_task);
		_task->m_taskchain = this;
		TRACE();
	}
	void taskchain::destroy()
	{
		while (!EVTask::empty())
		{
			TRACE();
			auto c = static_cast<task *>(EVTask::pop_front());
			c->taskext::m_taskchain = nullptr;
			c->destroy();
		}
	}
	taskchain::operator bool()
	{
		return !EVTask::empty();
	}
}