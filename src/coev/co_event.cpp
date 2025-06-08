/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <thread>
#include "co_event.h"
#include "local_resume.h"
#include "cv.h"

namespace coev
{
	void __ev_set_reserved(co_event &e, uint64_t x)
	{
		e.m_reserved = x;
	}
	uint64_t __ev_get_reserved(co_event &e)
	{
		return e.m_reserved;
	}
	co_event::co_event(queue *_ev_queue) : m_tid(gtid())
	{
		if (_ev_queue != nullptr)
		{
			_ev_queue->push_back(this);
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
		LOG_CORE("co_event resume m_caller:%p\n", m_caller ? m_caller.address() : 0);
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
			CV(m_caller).resume();
		}
		else
		{
			assert(false);
		}
	}
}
