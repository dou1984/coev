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
	struct task : EVTask, EVEvent
	{
		void insert_task(taskext *_task);
		void destroy();
		operator bool();
	};

}