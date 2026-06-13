/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "invalid.h"
#include "co_async.h"
#include "co_deliver.h"
#include "local.h"
namespace coev
{

	co_event co_async::suspend() noexcept
	{
		return co_event(this);
	}
	co_event co_async::suspend_util_next_loop() noexcept
	{
		return co_event(&local_async::instance());
	}
	bool co_async::resume(int64_t value) noexcept
	{
		if (auto c = static_cast<co_event *>(pop_front()); c != nullptr)
		{
			c->__set_reserved(value);
			c->resume();
			return true;
		}
		return false;
	}
	bool co_async::resume_next_loop() noexcept
	{
		if (auto c = static_cast<co_event *>(pop_front()); c != nullptr)
		{
			local_async::instance().push_back(c);
			return true;
		}
		return false;
	}
	int co_async::resume_all(int64_t e) noexcept
	{
		int count = 0;
		while (resume(e))
		{
			count += 1;
		}
		return count;
	}

	namespace guard
	{
		struct co_event_f : co_event
		{
			const std::function<void()> &m_getter;
			co_event_f(queue *__ev, const std::function<void()> &getter) : co_event(__ev), m_getter(getter)
			{
			}
		};
		co_event_f *co_async::__ev(const std::function<void()> &_set)
		{
			std::lock_guard<std::mutex> _(m_mutex);
			_set();
			auto c = static_cast<co_event_f *>(pop_front());
			if (c && c->m_getter)
			{
				c->m_getter();
			}
			return c;
		}
		co_async::~co_async()
		{
			if (local<is_destroying>::instance())
			{
				std::lock_guard<std::mutex> _(m_mutex);
				while (true)
				{
					auto c = static_cast<co_event_f *>(pop_front());
					if (c == nullptr)
					{
						break;
					}
					c->resume(INVALID);
				}
			}
		}
		awaitable<int64_t> co_async::suspend(const std::function<bool()> &_suspend, const std::function<void()> &_get) noexcept
		{
			int64_t value = 0;
			m_mutex.lock();
			if (_suspend())
			{
				co_event_f ev(this, _get);
				m_mutex.unlock();
				auto r = co_await ev;
				if (r == INVALID)
				{
					co_return r;
				}
				m_mutex.lock();
				value = ev.__get_reserved();
			}
			else if (_get)
			{
				_get();
			}
			m_mutex.unlock();
			co_return value;
		}
		bool co_async::deliver(const std::function<void()> &_set) noexcept
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
		bool co_async::resume(const std::function<void()> &_set) noexcept
		{
			if (auto c = __ev(_set); c != nullptr)
			{
				c->resume();
				return true;
			}
			return false;
		}
		bool co_async::deliver(uint64_t value) noexcept
		{
			auto _set = []() -> bool
			{ return false; };
			if (auto c = __ev(_set); c != nullptr)
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
