/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <chrono>
#include <thread>
#include "loop.h"
#include "waitgroup.h"

namespace coev
{
	int waitgroup::add(int c)
	{
		std::lock_guard<std::mutex> _(m_lock);
		m_count += c;
		return 0;
	}
	int waitgroup::done()
	{
		event *c = nullptr;
		{
			std::lock_guard<std::mutex> _(m_lock);
			if (--m_count > 0)
			{
				return 0;
			}
			else if (EVMutex::empty())
			{
				return 0;
			}
			else
			{
				c = static_cast<event *>(EVMutex::pop_front());
			}
		}
		loop::resume(c);
		return 0;
	}
	awaiter waitgroup::wait()
	{		
		m_lock.lock();
		EVMutex *ev = this;
		event _event(ev);
		m_lock.unlock();
		co_await _event;
		co_return 0;
	}
}