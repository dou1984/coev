#pragma once
#include <list>
#include "awaiter.h"
#include "waitfor.h"
#include "eventchainmutex.h"

namespace coev
{
	template <class TYPE>
	class channel : public EVMutex
	{
		std::list<TYPE> m_data;

	public:
		awaiter set(TYPE &&d)
		{
			EVMutex::lock();
			m_data.emplace_back(std::move(d));
			auto c = static_cast<event *>(EVMutex::pop_front());
			EVMutex::unlock();
			if (c)
			{
				c->resume();
			}
			co_return 0;
			/*
			EVMutex::resume(
				[this, &d]()
				{ m_data.emplace_back(std::move(d)); });
			co_return 0;
			*/
		}
		awaiter get(TYPE &d)
		{
			EVMutex::lock();
			if (m_data.empty())
			{
				co_await wait_for_x<EVMutex>(*this);
				EVMutex::lock();
			}
			d = std::move(m_data.front());
			m_data.pop_front();
			EVMutex::unlock();
			co_return 0;
			/*
			return EVMutex::wait_for(
				[this]()
				{ return m_data.empty(); },
				[this, &d]()
				{
					d = std::move(m_data.front());
					m_data.pop_front();
				});
			*/
		}
	};
}