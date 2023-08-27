/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <atomic>
#include <mutex>
#include "../coev.h"

namespace coev
{
	template <RESUME loop_resume>
	class waitgroupimpl : EVMutex
	{
	public:
		int add(int c = 1)
		{
			std::lock_guard<std::mutex> _(m_lock);
			m_count += c;
			return 0;
		}
		int done()
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
			loop_resume(c);
			return 0;
		}
		awaiter wait()
		{
			m_lock.lock();
			EVMutex *ev = this;
			event _event(ev);
			m_lock.unlock();
			co_await _event;
			co_return 0;
		}

	private:
		std::mutex m_lock;
		int m_count = 0;
	};
}