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
			co_await wait_for<EVMutex>(*this);
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
		auto c = static_cast<event *>(chain::pop_front());
		EVMutex::unlock();
		if (c)
		{
			c->resume();
		}
		co_return 0;
	}
}