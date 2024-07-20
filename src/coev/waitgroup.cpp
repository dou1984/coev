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
	awaiter<int> waitgroup::wait()
	{
		return wait_for_ts(m_trigger, []()
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
		while (resume_ts(m_trigger, []() {}))
		{
		}
		return 0;
	}
}
