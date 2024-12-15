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
		thread_safe::async m_listener;

	public:
		void move(TYPE &&d)
		{
			m_listener.resume(
				[this, d = std::move(d)]()
				{ m_data.emplace_back(std::move(d)); });
		}
		void set(const TYPE &d)
		{
			m_listener.resume(
				[this, d]()
				{ m_data.push_back(d); });
		}
		awaitable<TYPE> move()
		{
			TYPE d;
			co_await m_listener.suspend(
				[this]()
				{ return m_data.empty(); },
				[this, &d]()
				{
					d = std::move(m_data.front());
					m_data.pop_front();
				});
			co_return std::move(d);
		}
	};
}