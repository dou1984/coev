
#include "waitfor.h"

namespace coev
{
	event wait_for(trigger &_this)
	{
		return event(&_this);
	}

	bool resume(trigger &_this)
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
		awaiter wait_for(trigger &_this, const SUSPEND &suppend, const CALL &call)
		{
			_this.lock();
			if (suppend())
			{
				event ev(&_this);
				_this.unlock();
				co_await ev;
				_this.lock();
			}
			call();
			_this.unlock();
			co_return 0;
		}
		bool resume(trigger &_this, const CALL &call)
		{
			_this.lock();
			auto c = static_cast<event *>(_this.pop_front());
			call();
			_this.unlock();
			if (c)
			{
				c->resume();
				return true;
			}
			return false;
		}
	}
}