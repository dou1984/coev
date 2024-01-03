#include "waitgroup.h"
#include "waitfor.h"

namespace coev
{
	event waitgroup::wait()
	{
		EVMutex::lock();
		return wait_for<EVMutex>(*this);
	}
	int waitgroup::add(int c)
	{
		m_count += c;
		return 0;
	}
	int waitgroup::done()
	{
		if (--m_count > 0)
		{
			return 0;
		}
	__retry__:
		EVMutex::lock();
		auto c = static_cast<event *>(EVMutex::pop_front());
		EVMutex::unlock();
		if (c)
		{
			c->resume();
			goto __retry__;
		}
		return 0;
	}
}
