#include "async.h"
#include "co_deliver.h"
#include "local.h"
namespace coev
{
	extern void __set_reserved(co_event &, uint64_t x);
	extern uint64_t __get_reserved(co_event &);
	const std::function<void()> __empty_set = []() {};
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
	int async::resume_all()
	{
		int count = 0;
		while (resume())
		{
			count += 1;
		}
		return count;
	}
	namespace guard
	{
		co_event *async::__ev(const std::function<void()> &_set)
		{
			std::lock_guard<std::mutex> _(m_mutex);
			_set();
			return static_cast<co_event *>(pop_front());
		}
		awaitable<uint64_t> async::suspend(const std::function<bool()> &suppend, const std::function<void()> &_get)
		{
			uint64_t value = 0;
			m_mutex.lock();
			if (suppend())
			{
				co_event ev(this);
				m_mutex.unlock();
				co_await ev;
				m_mutex.lock();
				value = __get_reserved(ev);
			}
			_get();
			m_mutex.unlock();
			co_return value;
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
		bool async::resume(uint64_t value)
		{
			if (auto c = __ev(__empty_set); c != nullptr)
			{
				__set_reserved(*c, value);
				local<coev::async>::instance().push_back(c);
				return true;
			}
			return false;
		}

		bool async::deliver(uint64_t value)
		{
			if (auto c = __ev(__empty_set); c != nullptr)
			{
				__set_reserved(*c, value);
				co_deliver::resume(c);
				return true;
			}
		}
	}
}
