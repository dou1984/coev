/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "async.h"
#include "co_deliver.h"
#include "local.h"
namespace coev
{
	const std::function<void()> __empty_set = []() {};
	co_event async::suspend()
	{
		return co_event(this);
	}
	co_event async::suspend_util_next_loop()
	{
		return co_event(&local<coev::async>::instance());
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
			LOG_CORE("resume one event later");
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
		awaitable<uint64_t> async::suspend(const std::function<bool()> &_suppend, const std::function<void()> &_get)
		{
			uint64_t value = 0;
			m_mutex.lock();
			if (_suppend())
			{
				co_event ev(this);
				m_mutex.unlock();
				co_await ev;
				m_mutex.lock();
				value = ev.__get_reserved();
			}
			_get();
			m_mutex.unlock();
			co_return value;
		}
		bool async::resume(const std::function<void()> &_set)
		{
			if (auto c = __ev(_set); c != nullptr)
			{
				c->resume();
				return true;
			}
			return false;
		}
		bool async::resume(uint64_t value)
		{
			if (auto c = __ev(__empty_set); c != nullptr)
			{
				c->__set_reserved(value);
				c->resume();
				return true;
			}
			return false;
		}

		bool async::deliver(uint64_t value)
		{
			if (auto c = __ev(__empty_set); c != nullptr)
			{
				c->__set_reserved(value);
				if (c->id() == gtid())
				{
					c->resume();
				}
				else
				{
					co_deliver::resume(c);
				}
				return true;
			}
			return false;
		}
	}
}
