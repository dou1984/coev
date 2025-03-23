/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <unordered_map>
#include <mutex>
#include "co_deliver.h"
#include "cosys.h"

namespace coev
{
	void co_deliver::cb_async(struct ev_loop *loop, ev_async *w, int revents)
	{
		if (revents & EV_ASYNC)
		{
			co_deliver *_this = (co_deliver *)w->data;
			assert(_this != nullptr);
			_this->__resume();
			local<coev::async>::instance().resume_all();
		}
	}
	co_deliver::co_deliver()
	{
		__init();
	}
	void co_deliver::__init()
	{
		m_deliver.data = this;
		ev_async_init(&m_deliver, co_deliver::cb_async);
		m_tid = gtid();
		m_loop = cosys::data();		
		ev_async_start(m_loop, &m_deliver);		
	}
	co_deliver::~co_deliver()
	{
		if (m_loop)
		{
			ev_async_stop(m_loop, &m_deliver);
			m_loop = nullptr;
		}
	}
	int co_deliver::__done()
	{
		if (m_loop)
		{
			ev_async_send(m_loop, &m_deliver);
		}
		return 0;
	}
	int co_deliver::resume(co_event *ev)
	{
		std::lock_guard<std::mutex> _(m_lock);
		m_waiter.push_back(ev);
		return __done();
	}
	int co_deliver::__resume()
	{
		async _listener;
		{
			std::lock_guard<std::mutex> _(m_lock);
			m_waiter.moveto(&_listener);
		}
		while (_listener.resume(true))
		{
		}
		return 0;
	}
}