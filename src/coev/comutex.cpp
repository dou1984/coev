/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "comutex.h"
#include "waitfor.h"

namespace coev
{
	const int on = 1;
	const int off = 0;

	awaiter<int> comutex::lock()
	{
		return coev::wait_for_ts(
			m_trigger,
			[this]()
			{ return m_flag == on; },
			[this]()
			{ m_flag = on; });
	}
	bool comutex::unlock()
	{
		return resume_ts(
			m_trigger,
			[this]()
			{ m_flag = off; });
	}
}