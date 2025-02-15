/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "co_waitgroup.h"
#include "waitfor.h"

namespace coev
{
	awaitable<void> co_waitgroup::wait()
	{
		co_await m_waiter.suspend(
			[]()
			{ return true; }, []() {});
	}
	int co_waitgroup::add()
	{
		++m_count;
		return 0;
	}
	void co_waitgroup::done()
	{
		if (--m_count > 0)
		{
			return;
		}
		while (m_waiter.resume([]() {}))
		{
		}
	}
}
