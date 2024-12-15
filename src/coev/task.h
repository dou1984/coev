/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <memory>
#include <functional>
#include "chain.h"
#include "async.h"
#include "event.h"
#include "evtask.h"

namespace coev
{
	class task final
	{		
		async m_listener;
		thread_safe::async m_waiter;		
		
		void __erase(evtask* _notify);
	public:
		~task();
		void destroy();
		void done(evtask* _notify);
		bool empty();
		event wait();
		void insert(evtask* _notify);
	};
}
