
#include "waitfor.h"

namespace coev
{
	event wait_for(coev::async &_this)
	{
		return event(&_this);
	}

	bool notify(coev::async &_this)
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
		awaitable<void> wait_for(coev::ts::async &_this, const SUSPEND &suppend, CALL &&call)
		{
			_this.m_mutex.lock();
			if (suppend())
			{
				auto _call = std::move(call);
				event ev(&_this);
				_this.m_mutex.unlock();
				co_await ev;
				_this.m_mutex.lock();
				_call();
			} 
			else
			{
				call();
			}
			_this.m_mutex.unlock();
		}
		bool notify(coev::ts::async &_this, const CALL &call)
		{
			event *c = [&]() {
				std::lock_guard<std::mutex> _(_this.m_mutex);
				call();
				return static_cast<event *>(_this.pop_front());				
			}();
			if (c)
			{
				c->resume();
				return true;
			}		
			return false;
		}
	}

}