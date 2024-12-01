/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "waitgroup.h"
#include "waitfor.h"

namespace coev
{
	awaitable<void> waitgroup::wait()
	{
		co_await m_listener.suspend(
			[]()
			{ return true; }, []() {});
	}
	int waitgroup::add()
	{
		++m_count;
		return 0;
	}
	void waitgroup::done()
	{
		if (--m_count > 0)
		{
			return;
		}
		while (m_listener.resume([]() {}))
		{
		}
	}
}
