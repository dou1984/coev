/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <atomic>
#include "Awaiter.h"
#include "EventChain.h"
#include "Spinlock.h"

namespace coev
{
	struct Mutex : EVMutex
	{
		const int on = 1;
		const int off = 0;
		std::mutex m_lock;
		int m_flag = 0;
		Awaiter<int> lock();
		Awaiter<int> unlock();
	};
}