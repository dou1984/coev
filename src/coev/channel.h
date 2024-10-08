/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <list>
#include "awaitable.h"
#include "waitfor.h"
#include "async.h"

namespace coev
{
	template <class TYPE>
	class channel
	{
		std::list<TYPE> m_data;
		ts::async m_listener;

	public:
		void set(TYPE &&d)
		{
			ts::notify(
				m_listener,
				[this, d = std::move(d)]()
				{ m_data.emplace_back(std::move(d)); });
		}
		void set(const TYPE &d)
		{
			ts::notify(
				m_listener,
				[this, d]()
				{ m_data.emplace_back(std::move(d)); });
		}
		awaitable<void> get(TYPE &d)
		{
			return ts::wait_for(
				m_listener,
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