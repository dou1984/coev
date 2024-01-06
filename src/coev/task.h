/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include "chain.h"
#include "eventchain.h"
#include "eventchain.h"
#include "taskevent.h"

namespace coev
{
	struct task final : EVTask, EVEvent
	{
		virtual ~task();
		void insert_task(taskevent *_task);
		void destroy();
		bool empty() { return EVTask::empty(); }
	};
}
