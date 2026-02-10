/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <cassert>
#include "co_mutex.h"
#include "wait_for.h"

namespace coev
{
	const int on = 1;
	const int off = 0;

	awaitable<void> co_mutex::lock()
	{
		if (m_flag == on)
		{
			co_await m_waiter.suspend();
			assert(m_flag == off);
		}
		m_flag = on;
	}
	bool co_mutex::unlock()
	{
		assert(m_flag == on);
		m_flag = off;
		return m_waiter.resume();
	}
	bool co_mutex::try_lock()
	{
		if (m_flag == on)
		{
			return false;
		}
		m_flag = on;
		return true;
	}

	namespace guard
	{
		awaitable<void> co_mutex::lock()
		{
			co_await m_waiter.suspend(
				[this]()
				{ return m_flag == on; }, [this]()
				{ m_flag = on; });
		}
		bool co_mutex::unlock()
		{
			return m_waiter.resume(
				[this]()
				{ m_flag = off; });
		}
		bool co_mutex::try_lock()
		{
			std::lock_guard<std::mutex> lock(m_waiter.lock());
			if (m_flag == on)
			{
				return false;
			}
			m_flag = on;
			return true;
		}
	}
}
