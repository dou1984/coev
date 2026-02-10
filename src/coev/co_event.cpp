/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <thread>
#include "co_event.h"
#include "local_resume.h"
#include "x.h"

namespace coev
{
	void co_event::__set_reserved(uint64_t x)
	{
		m_reserved = x;
	}
	uint64_t co_event::__get_reserved()
	{
		return m_reserved;
	}
	co_event::co_event(queue *__ev, bool __seq) : m_tid(gtid())
	{
		if (__ev != nullptr)
		{
			__seq ? __ev->push_back(this) : __ev->push_front(this);
		}
	}

	co_event::~co_event()
	{
		if (!queue::empty())
		{
			queue::erase(this);
		}
		m_caller = nullptr;
	}
	void co_event::await_resume()
	{
	}
	bool co_event::await_ready()
	{
		return m_status.load(std::memory_order_consume);
	}
	void co_event::await_suspend(std::coroutine_handle<> _awaitable)
	{
		m_caller = _awaitable;
		int e0 = CORO_INIT;
		int e1 = CORO_RESUMED;
		if (m_status.compare_exchange_strong(e0, CORO_SUSPEND, std::memory_order_acquire, std::memory_order_acquire))
		{
		}
		else if (m_status.compare_exchange_strong(e1, CORO_FINISHED, std::memory_order_acquire, std::memory_order_acquire))
		{
			__resume();
		}
		else
		{
			assert(false);
		}
	}
	void co_event::resume()
	{
		LOG_CORE("co_event resume m_caller:%p", m_caller ? m_caller.address() : 0);
		int e0 = CORO_INIT;
		int e1 = CORO_SUSPEND;
		if (m_status.compare_exchange_strong(e0, CORO_RESUMED, std::memory_order_acquire, std::memory_order_acquire))
		{
		}
		else if (m_status.compare_exchange_strong(e1, CORO_FINISHED, std::memory_order_acquire, std::memory_order_acquire))
		{
			__resume();
		}
		else
		{
			assert(false);
		}
	}
	void co_event::__resume()
	{
		if (m_caller.address() && !m_caller.done())
		{
			X(m_caller).resume();
		}
		else
		{
			assert(false);
		}
	}
}
