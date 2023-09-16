#include "comutex.h"

namespace coev
{
	awaiter comutex::lock()
	{
		m_lock.lock();
		if (m_flag == off)
		{
			m_lock.unlock();
			co_return 0;
		}
		EVRecv *ev = this;
		event _event(ev);
		m_lock.unlock();
		co_await _event;
		co_return 0;
	}
	awaiter comutex::unlock()
	{
		std::lock_guard<std::recursive_mutex> _(m_lock);
		if (m_flag == off)
		{
		}
		else if (EVRecv::empty())
		{
			m_flag = off;
		}
		else
		{
			auto c = static_cast<event *>(EVRecv::pop_front());
			loop::resume(c);
		}
		co_return 0;
	}
}