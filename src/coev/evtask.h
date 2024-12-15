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
	class task;
	struct evtask : chain
	{
		task *m_task = nullptr;
		virtual ~evtask();	
		virtual void destroy();
		void resume();
	};

}