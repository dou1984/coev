/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include "Chain.h"
#include "Awaiter.h"
#include "EventChain.h"

namespace coev
{
	struct TaskExt;
	struct TaskSet;

	using Task = Awaiter<int, TaskExt>;
	struct TaskSet : EVTask, EVEvent
	{
		void insert_task(TaskExt *_task);
		void destroy();
		operator bool();
	};
	struct TaskExt : Chain
	{
		TaskSet *m_TaskSet = nullptr;
		void resume_ex();
		virtual ~TaskExt();
	};

}