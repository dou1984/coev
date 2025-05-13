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
#include "wait_for.h"
#include "async.h"

namespace coev
{
	template <class TYPE>
	class co_channel
	{
	public:
		void move(TYPE &&d)
		{
			m_waiter.resume(
				[this, d = std::move(d)]()
				{ m_data.emplace_back(std::move(d)); });
		}
		void set(const TYPE &d)
		{
			m_waiter.resume(
				[this, d]()
				{ m_data.push_back(d); });
		}
		awaitable<TYPE> move()
		{
			TYPE d;
			co_await m_waiter.suspend(
				[this]()
				{ return __invalid(); },
				[&]()
				{ d = __pop_front(); });
			co_return d;
		}

	private:
		std::list<TYPE> m_data;
		guard::async m_waiter;
		bool __invalid() const { return m_data.empty(); }
		TYPE __pop_front()
		{
			auto d = std::move(m_data.front());
			m_data.pop_front();
			return d;
		}
	};
}