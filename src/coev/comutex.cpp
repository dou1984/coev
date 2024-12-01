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

	awaitable<void> comutex::lock()
	{
		co_await m_listener.suspend(
			[this]()
			{ return m_flag == on; },
			[this]()
			{ m_flag = on; });
	}
	bool comutex::unlock()
	{
		return m_listener.resume(
			[this]()
			{ m_flag = off; });
	}
}
