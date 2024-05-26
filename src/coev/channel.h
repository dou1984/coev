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
#include "trigger.h"

namespace coev
{
	template <class TYPE>
	class channel
	{
		std::list<TYPE> m_data;
		ts::trigger m_trigger;

	public:
		awaiter set(TYPE &&d)
		{
			ts::resume(
				m_trigger,
				[this, d = std::move(d)]()
				{ m_data.emplace_back(std::move(d)); });
			co_return 0;
		}
		awaiter set(const TYPE &d)
		{
			ts::resume(
				m_trigger,
				[this, d]()
				{ m_data.emplace_back(std::move(d)); });
			co_return 0;
		}
		awaiter get(TYPE &d)
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