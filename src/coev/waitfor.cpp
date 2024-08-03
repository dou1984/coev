
#include "waitfor.h"

namespace coev
{
	event wait_for(async &_this)
	{
		return event(&_this);
	}

	bool trigger(async &_this)
	{
		auto c = static_cast<event *>(_this.pop_front());
		if (c)
		{
			c->resume();
			return true;
		}
		return false;
	}

	namespace ts
	{
		awaitable<void> wait_for(ts::async &_this, const SUSPEND &suppend, const CALL &call)
		{
			_this.m_mutex.lock();
			if (suppend())
			{
				event ev(&_this);
				_this.m_mutex.unlock();
				co_await ev;
				_this.m_mutex.lock();
			}
			call();
			_this.m_mutex.unlock();
		}
		bool trigger(ts::async &_this, const CALL &call)
		{
			event *c = nullptr;
			{
				std::lock_guard<std::mutex> _(_this.m_mutex);
				c = static_cast<event *>(_this.pop_front());
				if (c == nullptr)
				{
					return false;
				}
				call();
			}
			c->resume();
			return true;
		}
	}

}