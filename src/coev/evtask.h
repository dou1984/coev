/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <functional>
#include "chain.h"

namespace coev
{
	class co_task;
	class evtask : public chain
	{
		friend class co_task;
		co_task *m_task = nullptr;
		int m_id = 0;

	public:
		evtask() = default;
		evtask(co_task *, int);
		virtual ~evtask();
		virtual void destroy();
		void resume();
	};

}