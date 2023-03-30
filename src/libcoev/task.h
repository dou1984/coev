/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include "chain.h"
#include "awaiter.h"
#include "eventchain.h"

namespace coev
{
	struct taskext;
	struct taskchain;

	using task = awaiter<int, taskext>;
	struct taskchain : EVTask, EVEvent
	{
		void insert_task(taskext *_task);
		void destroy();
		operator bool();
	};
	struct taskext : chain
	{
		taskchain *m_taskchain = nullptr;
		void resume_ex();
		virtual ~taskext();
	};

}