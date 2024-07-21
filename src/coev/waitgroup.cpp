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
	awaiter<void> waitgroup::wait()
	{
		return ts::wait_for(m_trigger, []()
						   { return true; }, []() {});
	}
	int waitgroup::add(int c)
	{
		m_count += c;
		return 0;
	}
	int waitgroup::done()
	{
		if (--m_count > 0)
		{
			return 0;
		}
		while (ts::resume(m_trigger, []() {}))
		{
		}
		return 0;
	}
}
