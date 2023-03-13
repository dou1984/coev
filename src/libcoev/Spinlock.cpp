/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <chrono>
#include <thread>
#include "Spinlock.h"
namespace coev
{
	bool Spinlock::try_lock()
	{
		int t = off;
		return flag.compare_exchange_strong(t, on);
	}
	void Spinlock::lock(int t)
	{
		while (!try_lock())
			if (t > 0)
				std::this_thread::sleep_for(std::chrono::microseconds(t));
	}
	void Spinlock::unlock()
	{
		int t = on;
		flag.compare_exchange_weak(t, off);
	}
}