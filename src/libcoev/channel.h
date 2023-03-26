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
#include "Loop.h"
#include "event.h"
#include "eventchain.h"

namespace coev
{
	template <class TYPE>
	class channel : EVChannel
	{
		std::list<TYPE> m_data;
		std::mutex m_lock;
		void __get(TYPE &d)
		{
			assert(m_data.size());
			d = std::move(m_data.front());
			m_data.pop_front();
		}
		event *__set(TYPE &d)
		{
			std::lock_guard<decltype(m_lock)> _(m_lock);
			m_data.emplace_back(std::move(d));
			return !EVChannel::empty() ? static_cast<event *>(EVChannel::pop_front()) : nullptr;
		}
	public:
		awaiter<int> set(TYPE &d)
		{
			if (auto c = __set(d))
			{
				Loop::resume(c);
			}
			co_return 0;
		}
		awaiter<int> get(TYPE &d)
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
				event e(static_cast<EVChannel *>(this), Loop::tag());
				m_lock.unlock();
				co_await e;
			}
		}
	};
}
