/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <atomic>
#include "co_async.h"
#include "awaitable.h"

namespace coev
{
	class co_waitgroup final
	{
		guard::co_async m_waiter;
		std::atomic_int m_count{0};

	public:
		int add();
		int add(int n);
		void done();
		awaitable<void> wait();
	};
}