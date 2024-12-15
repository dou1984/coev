/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <unordered_map>
#include <mutex>
#include "awaken.h"
#include "cosys.h"

namespace coev
{
	void awaken::cb_async(struct ev_loop *loop, ev_async *w, int revents)
	{
		if (revents & EV_ASYNC)
		{
			awaken *_this = (awaken *)w->data;
			assert(_this != nullptr);
			_this->__resume();
		}
	}
	awaken::awaken()
	{
		__init();
	}
	void awaken::__init()
	{
		m_awaken.data = this;
		ev_async_init(&m_awaken, awaken::cb_async);
		m_tid = gtid();
		m_loop = cosys::data();		
		ev_async_start(m_loop, &m_awaken);		
	}
	awaken::~awaken()
	{
		if (m_loop)
		{
			ev_async_stop(m_loop, &m_awaken);
			m_loop = nullptr;
		}
	}
	int awaken::__done()
	{
		if (m_loop)
		{
			ev_async_send(m_loop, &m_awaken);
		}
		return 0;
	}
	int awaken::resume(event *ev)
	{
		std::lock_guard<std::mutex> _(m_lock);
		m_listener.push_back(ev);
		return __done();
	}
	int awaken::__resume()
	{
		async _listener;
		{
			std::lock_guard<std::mutex> _(m_lock);
			m_listener.moveto(&_listener);
		}
		while (_listener.resume())
		{
		}
		return 0;
	}
}