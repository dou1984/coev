/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <chrono>
#include <thread>
#include "Loop.h"
#include "Mutex.h"

namespace coev
{
	awaiter<int> Mutex::lock()
	{
		m_lock.lock();
		if (m_flag == off)
		{
			m_flag = on;
			m_lock.unlock();
			co_return 0;
		}
		EVMutex *ev = this;
		event _event(ev);
		m_lock.unlock();
		co_await _event;
		co_return 0;
	}
	awaiter<int> Mutex::unlock()
	{
		m_lock.lock();
		if (m_flag == off)
		{
			m_lock.unlock();
		}
		else if (EVMutex::empty())
		{
			m_flag = off;
			m_lock.unlock();
		}
		else
		{
			auto c = static_cast<event *>(EVMutex::pop_front());
			m_lock.unlock();
			Loop::resume(c);
		}
		co_return 0;
	}
}