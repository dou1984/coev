#include "async.h"

namespace coev
{

	co_event async::suspend()
	{
		return co_event(this);
	}

	bool async::resume(bool immediately)
	{
		if (auto c = static_cast<co_event *>(pop_front()); c != nullptr)
		{
			if (immediately)
			{
				c->resume();				
			}
			else
			{
				local<coev::async>::instance().push_back(c);
			}
			return true;
		}
		return false;
	}
	void async::resume_all()
	{
		while (resume(true))
		{
			LOG_CORE("resume one\n");
		}
	}

	namespace guard
	{
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
			auto c = [&, this]()
			{
				std::lock_guard<std::mutex> _(m_mutex);
				_set();
				return static_cast<co_event *>(pop_front());
			}();
			if (c)
			{
				LOG_CORE("resume %p\n", c);
				c->resume();
				return true;
			}
			return false;
		}
	}
}