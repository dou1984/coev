#include "comutex.h"
#include "async.h"
#include "waitfor.h"

namespace coev
{
	const int on = 1;
	const int off = 0;

	awaiter comutex::lock()
	{
		return coev::wait_for(
			this,
			[this]()
			{ return m_flag == on; },
			[this]()
			{ m_flag = on; });
	}
	awaiter comutex::unlock()
	{
		resume(
			this,
			[this]()
			{ m_flag = off; });
		co_return 0;
	}
}