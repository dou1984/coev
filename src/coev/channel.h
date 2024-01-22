#pragma once
#include <list>
#include "awaiter.h"
#include "waitfor.h"
#include "evlist.h"

namespace coev
{
	template <class TYPE>
	class channel : public async_mutex
	{
		std::list<TYPE> m_data;

	public:
		awaiter set(TYPE &&d)
		{
			ts::resume(
				this,
				[this, &d]()
				{ m_data.emplace_back(std::move(d)); });
			co_return 0;
		}
		awaiter set(const TYPE &d)
		{
			ts::resume(
				this,
				[this, d]()
				{ m_data.emplace_back(std::move(d)); });
			co_return 0;
		}
		awaiter get(TYPE &d)
		{
			return ts::wait_for(
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