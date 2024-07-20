/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <list>
#include "awaiter.h"
#include "waitfor.h"
#include "async.h"

namespace coev
{
	template <class TYPE>
	class channel
	{
		std::list<TYPE> m_data;
		async_ts m_trigger;

	public:
		awaiter<int> set(TYPE &&d)
		{
			resume_ts(
				m_trigger,
				[this, d = std::move(d)]()
				{ m_data.emplace_back(std::move(d)); });
			co_return 0;
		}
		awaiter<int> set(const TYPE &d)
		{
			resume_ts(
				m_trigger,
				[this, d]()
				{ m_data.emplace_back(std::move(d)); });
			co_return 0;
		}
		awaiter<int> get(TYPE &d)
		{
			return wait_for_ts(
				m_trigger,
				[this]()
				{ return m_data.empty(); },
				[this, &d]()
				{
					d = std::move(m_data.front());
					m_data.pop_front();
				});
		}
	};
}