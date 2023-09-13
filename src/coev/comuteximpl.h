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
#include "awaiter.h"

namespace coev
{
	template <RESUME loop_resume>
	class comuteximpl : EVRecv
	{
	public:
		awaiter lock()
		{
			m_lock.lock();
			if (m_flag == off)
			{
				m_lock.unlock();
				co_return 0;
			}
			EVRecv *ev = this;
			event _event(ev);
			m_lock.unlock();
			co_await _event;
			co_return 0;
		}
		awaiter unlock()
		{
			std::lock_guard<std::recursive_mutex> _(m_lock);
			if (m_flag == off)
			{
			}
			else if (EVRecv::empty())
			{
				m_flag = off;
			}
			else
			{
				auto c = static_cast<event *>(EVRecv::pop_front());
				loop_resume(c);
			}
			co_return 0;
		}

	private:
		const int on = 1;
		const int off = 0;
		std::recursive_mutex m_lock;
		int m_flag = 0;
	};
}