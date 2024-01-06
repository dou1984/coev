#include "comutex.h"
#include "waitfor.h"

namespace coev
{
	const int on = 1;
	const int off = 0;

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
}