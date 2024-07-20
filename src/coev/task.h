/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include "chain.h"
#include "async.h"
#include "taskevent.h"

namespace coev
{
	struct task final
	{
		async m_trigger;
		async_ts m_trigger_mutex;
		virtual ~task();
		void insert_task(taskevent *_task);
		void erase_task(taskevent *_task);
		void destroy();
		bool empty();
	};
}
