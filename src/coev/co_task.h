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
		guard::async m_waiter;
		std::unordered_map<promise *, uint64_t> m_promises;
		uint64_t m_id = 0;

	public:
		virtual ~co_task();
		void destroy();

		int operator<<(promise *);
		int operator>>(promise *);
		int load(promise *);
		int unload(promise *);
		bool empty();
		awaitable<void> wait_all();
		awaitable<uint64_t> wait();
	};
}
