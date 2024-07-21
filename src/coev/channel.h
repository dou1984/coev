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
		ts::async m_trigger;

	public:
		awaiter<int> set(TYPE &&d)
		{
			ts::resume(
				m_trigger,
				[this, d = std::move(d)]()
				{ m_data.emplace_back(std::move(d)); });
			co_return 0;
		}
		awaiter<int> set(const TYPE &d)
		{
			ts::resume(
				m_trigger,
				[this, d]()
				{ m_data.emplace_back(std::move(d)); });
			co_return 0;
		}
		awaiter<void> get(TYPE &d)
		{
			return ts::wait_for(
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