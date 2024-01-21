#pragma once
#include <list>
#include "awaiter.h"
#include "waitfor.h"
#include "eventchain.h"

namespace coev
{
	template <class TYPE>
	class channel : public EVMutex
	{
		std::list<TYPE> m_data;

	public:
		awaiter set(TYPE &&d)
		{
			ts::resume<EVMutex>(this,
								  [this, &d]()
								  { m_data.emplace_back(std::move(d)); });
			co_return 0;
		}
		awaiter set(const TYPE &d)
		{
			ts::resume<EVMutex>(this,
								  [this, d]()
								  { m_data.emplace_back(std::move(d)); });
			co_return 0;
		}
		awaiter get(TYPE &d)
		{
			return ts::wait_for<EVMutex>(
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