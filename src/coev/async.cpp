#include "async.h"

namespace coev
{

	event async::suspend()
	{
		return event(this);
	}

	bool async::resume()
	{
		if (auto c = static_cast<event *>(pop_front()); c != nullptr)
		{
			c->resume();
			return true;
		}
		return false;
	}

	namespace thread_safe
	{
		awaitable<void> async::suspend(const std::function<bool()> &suppend, std::function<void()> &&call)
		{
			m_mutex.lock();
			if (suppend())
			{
				event ev(this);
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
				return static_cast<event *>(pop_front());
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