#pragma once
#include "../coev.h"

namespace coev
{
	template <class TYPE>
	class channel : public EVRecv
	{
		std::list<TYPE> m_data;
		std::recursive_mutex m_lock;
		void __get(TYPE &d)
		{
			assert(m_data.size());
			d = std::move(m_data.front());
			m_data.pop_front();
		}
		event *__set(TYPE &&d)
		{
			std::lock_guard<std::recursive_mutex> _(m_lock);
			m_data.emplace_back(std::move(d));
			return static_cast<event *>(EVRecv::pop_front());
		}

	public:
		awaiter set(TYPE &&d)
		{
			if (auto ev = __set(std::move(d)))
			{
				loop::resume(ev);
			}
			co_return 0;
		}
		awaiter get(TYPE &d)
		{
			while (true)
			{
				m_lock.lock();
				if (!m_data.empty())
				{
					__get(d);
					m_lock.unlock();
					co_return 0;
				}
				EVRecv *ev = this;
				event _event(ev, ttag());
				m_lock.unlock();
				co_await _event;
			}
		}
	};
}