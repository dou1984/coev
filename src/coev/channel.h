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
	class channel : public async<evlts>
	{
		std::list<TYPE> m_data;

	public:
		awaiter set(TYPE &&d)
		{
			resume(
				this,
				[this, d = std::move(d)]()
				{ m_data.emplace_back(std::move(d)); });
			co_return 0;
		}
		awaiter set(const TYPE &d)
		{
			resume(
				this,
				[this, d]()
				{ m_data.emplace_back(std::move(d)); });
			co_return 0;
		}
		awaiter get(TYPE &d)
		{
			return wait_for(
				this,
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