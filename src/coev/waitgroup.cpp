#include "waitgroup.h"
#include "waitfor.h"

namespace coev
{
	awaiter waitgroup::wait()
	{
		return wait_for(this,
			[]()
			{ return true; },
			[]() {});
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
		while (resume(this, []() {}))
		{
		}
		return 0;
	}
}
