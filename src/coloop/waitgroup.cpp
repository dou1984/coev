#include "waitgroup.h"

namespace coev
{
	int waitgroup::add(int c)
	{
		std::lock_guard<std::recursive_mutex> _(m_lock);
		m_count += c;
		return 0;
	}
	int waitgroup::done()
	{
		std::lock_guard<std::recursive_mutex> _(m_lock);
		if (--m_count > 0)
		{
		}
		else
		{
			while (auto c = static_cast<event *>(EVRecv::pop_front()))
			{
				loop::resume(c);
			}
		}
		return 0;
	}
	event waitgroup::wait()
	{
		std::lock_guard<std::recursive_mutex> _(m_lock);
		EVRecv *ev = this;
		return event{ev};
	}
}