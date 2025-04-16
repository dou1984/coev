#include "async.h"
#include "co_deliver.h"

namespace coev
{
	co_event async::suspend()
	{
		return co_event(this);
	}
	bool async::resume()
	{
		if (auto c = static_cast<co_event *>(pop_front()); c != nullptr)
		{
			// LOG_CORE("resume one event immediately\n");
			c->resume();
			return true;
		}
		return false;
	}
	bool async::resume_next_loop()
	{
		if (auto c = static_cast<co_event *>(pop_front()); c != nullptr)
		{
			LOG_CORE("resume one event later\n");
			local<coev::async>::instance().push_back(c);
			return true;
		}
		return false;
	}
	bool async::resume_all()
	{
		bool ok = false;
		while (resume())
		{
			ok = true;
		}
		return ok;
	}
	namespace guard
	{
		co_event *async::__ev(const std::function<void()> &_set)
		{
			std::lock_guard<std::mutex> _(m_mutex);
			_set();
			return static_cast<co_event *>(pop_front());
		}
		awaitable<void> async::suspend(const std::function<bool()> &suppend, const std::function<void()> &_get)
		{
			m_mutex.lock();
			if (suppend())
			{
				co_event ev(this);
				m_mutex.unlock();
				co_await ev;
				m_mutex.lock();
			}
			_get();
			m_mutex.unlock();
		}
		bool async::resume(const std::function<void()> &_set)
		{
			if (auto c = __ev(_set); c != nullptr)
			{
				local<coev::async>::instance().push_back(c);
				return true;
			}
			return false;
		}
		bool async::deliver(const std::function<void()> &_set)
		{
			if (auto c = __ev(_set); c != nullptr)
			{
				co_deliver::resume(c);
				return true;
			}
			return false;
		}
	}
}