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
		co_await ts::wait_for(m_listener, []() { return true; }, []() {});
	}
	int waitgroup::add(int c)
	{
		m_count += c;
		return 0;
	}
	void waitgroup::done()
	{
		if (--m_count <= 0 )
		{
			return ;
		}
		while (ts::notify(m_listener, []() {}))
		{
		}	
	}
}
