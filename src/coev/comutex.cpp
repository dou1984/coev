#include "comutex.h"
#include "waitfor.h"

namespace coev
{
	const int on = 1;
	const int off = 0;
	
	awaiter comutex::lock()
	{
		EVMutex::lock();
		if (m_flag == on)
		{
			co_await wait_for_x<EVMutex>(*this);
			EVMutex::lock();
		}
		m_flag = on;
		EVMutex::unlock();
		co_return 0;
	}
	awaiter comutex::unlock()
	{
		EVMutex::lock();
		m_flag = off;
		auto c = static_cast<event *>(EVMutex::pop_front());
		EVMutex::unlock();
		if (c)
		{
			c->resume();
		}
		co_return 0;
	}
	/*
	awaiter comutex::lock()
	{
		return EVMutex::wait_for(
			[this]()
			{ return m_flag == on; },
			[this]()
			{ m_flag = on; });
	}
	awaiter comutex::unlock()
	{
		EVMutex::resume(
			[this]()
			{ m_flag = off; });
		co_return 0;
	}
	*/
}