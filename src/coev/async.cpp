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
	co_event async::suspend() noexcept
	{
		return co_event(this);
	}
	co_event async::suspend_util_next_loop() noexcept
	{
		return co_event(&local_async::instance());
	}
	bool async::resume(uint64_t value) noexcept
	{
		if (auto c = static_cast<co_event *>(pop_front()); c != nullptr)
		{
			c->__set_reserved(value);
			c->resume();
			return true;
		}
		return false;
	}
	bool async::resume_next_loop() noexcept
	{
		if (auto c = static_cast<co_event *>(pop_front()); c != nullptr)
		{
			local_async::instance().push_back(c);
			return true;
		}
		return false;
	}
	int async::resume_all() noexcept
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
		co_event *async::__ev() noexcept
		{
			std::lock_guard<std::mutex> _(m_mutex);
			return static_cast<co_event *>(pop_front());
		}
		awaitable<uint64_t> async::suspend(const std::function<bool()> &_suspend, const std::function<void()> &_get) noexcept
		{
			uint64_t value = 0;
			m_mutex.lock();
			if (_suspend())
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
		awaitable<uint64_t> async::suspend() noexcept
		{
			uint64_t value = 0;
			m_mutex.lock();
			co_event ev(this);
			m_mutex.unlock();
			co_await ev;
			m_mutex.lock();
			value = ev.__get_reserved();
			m_mutex.unlock();
			co_return value;
		}
		bool async::resume(const std::function<void()> &_set) noexcept
		{
			if (auto c = __ev(_set); c != nullptr)
			{
				c->resume();
				return true;
			}
			return false;
		}
		bool async::resume(uint64_t value) noexcept
		{
			if (auto c = __ev(); c != nullptr)
			{
				c->__set_reserved(value);
				c->resume();
				return true;
			}
			return false;
		}
		bool async::deliver(const std::function<void()> &_set) noexcept
		{
			if (auto c = __ev(_set); c != nullptr)
			{
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
		bool async::deliver(uint64_t value) noexcept
		{
			if (auto c = __ev(); c != nullptr)
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
