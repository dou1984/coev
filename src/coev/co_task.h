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
#include "queue.h"
#include "async.h"
#include "event.h"
#include "evtask.h"

namespace coev
{
	class co_task final
	{
		guard::async m_task_waiter;
		guard::async m_ev_waiter;
		int m_last = 0;
		void __erase(evtask *_notify);

	public:
		~co_task();
		void destroy();
		void done(evtask *_notify);
		bool empty();
		awaitable<int> wait();
		void insert( int _id, evtask *_notify);
	};
}
