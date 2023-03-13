/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <coroutine>
#include <list>
#include <mutex>
#include "Loop.h"
#include "Event.h"
#include "EventSet.h"

namespace coev
{
	template <class TYPE>
	class Channel : EVChannel
	{
		std::list<TYPE> m_data;
		std::mutex m_lock;
		void __get(TYPE &d)
		{
			assert(m_data.size());
			d = std::move(m_data.front());
			m_data.pop_front();
		}
		Event *__set(TYPE &d)
		{
			std::lock_guard<decltype(m_lock)> _(m_lock);
			m_data.emplace_back(std::move(d));
			return !EVChannel::empty() ? static_cast<Event *>(EVChannel::pop_front()) : nullptr;
		}
	public:
		Awaiter<int> set(TYPE &d)
		{
			if (auto c = __set(d))
			{
				Loop::resume(c);
			}
			co_return 0;
		}
		Awaiter<int> get(TYPE &d)
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
				Event e(static_cast<EVChannel *>(this), Loop::tag());
				m_lock.unlock();
				co_await e;
			}
		}
	};
}
