/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <atomic>
#include <cstddef>

namespace coev
{
	struct spinlock
	{
		const int on = 1;
		const int off = 0;
		std::atomic<int> flag{off};

		bool try_lock();
		void lock(int t = 0);
		void unlock();
	};
}