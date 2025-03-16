#include "async.h"

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
			c->resume();
			return true;
		}
		return false;
	}

	namespace guard
	{
		awaitable<void> async::suspend(const std::function<bool()> &suppend, const std::function<void()> &call)
		{
			m_mutex.lock();
			if (suppend())
			{
				co_event ev(this);
				m_mutex.unlock();
				co_await ev;
				m_mutex.lock();
			}
			call();
			m_mutex.unlock();
		}
		bool async::resume(const std::function<void()> &call)
		{
			auto c = [&]()
			{
				std::lock_guard<std::mutex> _(m_mutex);
				call();
				return static_cast<co_event *>(pop_front());
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