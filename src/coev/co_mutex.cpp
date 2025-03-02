/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "co_mutex.h"
#include "wait_for.h"

namespace coev::guard
{
	const int on = 1;
	const int off = 0;

	awaitable<void> co_mutex::lock()
	{
		co_await m_waiter.suspend(
			[this]()
			{ return m_flag == on; },
			[this]()
			{ m_flag = on; });
	}
	bool co_mutex::unlock()
	{
		return m_waiter.resume(
			[this]()
			{ m_flag = off; });
	}
}
