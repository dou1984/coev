/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <coroutine>
#include <list>
#include <mutex>
#include "awaiter.h"

namespace coev
{
	
	template <class TYPE, RESUME loop_resume>
	class channelimpl : EVRecv
	{
		std::list<TYPE> m_data;
		std::mutex m_lock;
		void __get(TYPE &d)
		{
			assert(m_data.size());
			d = std::move(m_data.front());
			m_data.pop_front();
		}
		event *__set(TYPE &&d)
		{
			std::lock_guard<decltype(m_lock)> _(m_lock);
			m_data.emplace_back(std::move(d));
			return !EVRecv::empty() ? static_cast<event *>(EVRecv::pop_front()) : nullptr;
		}

	public:
		awaiter set(TYPE &&d)
		{
			if (auto ev = __set(std::move(d)))
			{
				loop_resume(ev);
			}
			co_return 0;
		}
		awaiter get(TYPE &d)
		{
			while (true)
			{
				m_lock.lock();
				if (!m_data.empty())
				{
					__get(d);
					m_lock.unlock();
					co_return 0;
				}
				event ev(static_cast<EVRecv *>(this), ttag());
				m_lock.unlock();
				co_await ev;
			}
		}
	};
}