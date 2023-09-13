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
	class waitgroupimpl : EVRecv
	{
	public:
		int add(int c = 1)
		{
			std::lock_guard<std::recursive_mutex> _(m_lock);
			m_count += c;
			return 0;
		}
		int done()
		{
			std::lock_guard<std::recursive_mutex> _(m_lock);
			if (--m_count > 0)
			{
			}
			else if (EVRecv::empty())
			{
			}
			else
			{
				auto c = static_cast<event *>(EVRecv::pop_front());
				loop_resume(c);
			}
			return 0;
		}
		awaiter wait()
		{
			m_lock.lock();
			EVRecv *ev = this;
			event _event(ev);
			m_lock.unlock();
			co_await _event;
			co_return 0;
		}

	private:
		std::recursive_mutex m_lock;
		int m_count = 0;
	};
}