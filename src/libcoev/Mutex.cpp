#include <chrono>
#include <thread>
#include "Loop.h"
#include "Mutex.h"

namespace coev
{
	Awaiter<int> Mutex::lock()
	{
		m_lock.lock();
		if (m_flag == off)
		{
			m_flag = on;
			m_lock.unlock();
			co_return 0;
		}
		EVMutex *ev = this;
		Event _event(ev, Loop::tag());
		m_lock.unlock();
		co_await _event;
		co_return 0;
	}
	Awaiter<int> Mutex::unlock()
	{
		m_lock.lock();
		if (m_flag == off)
		{
			m_lock.unlock();
		}
		else if (EVMutex::empty())
		{
			m_flag = off;
			m_lock.unlock();
		}
		else
		{
			auto c = static_cast<Event *>(EVMutex::pop_front());
			m_lock.unlock();
			Loop::resume(c);
		}
		co_return 0;
	}
}