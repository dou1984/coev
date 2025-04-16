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
#include "co_event.h"
#include "is_destroying.h"

namespace coev
{
#define co_start coev::local<coev::co_task>::instance() 

	class co_task final
	{
		guard::async m_task_waiter;
		std::vector<promise *> m_promises;
		int m_count = 0;
		int m_id = 0;

	public:
		virtual ~co_task();
		void destroy();

		int operator<<(promise *);
		int operator>>(promise *);
		int release(promise *);
		bool empty();
		awaitable<void> wait_all();
		awaitable<int> wait();
	};
}
