/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
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
	awaken::awaken() : awaken(cosys::data(), gtid())
	{
	}
	awaken::awaken(struct ev_loop *__loop, uint64_t __tag)
	{
		m_awaken.data = this;
		ev_async_init(&m_awaken, awaken::cb_async);
		ev_async_start(__loop, &m_awaken);
		m_tid = __tag;
	}
	awaken::~awaken()
	{
		ev_async_stop(cosys::at(m_tid), &m_awaken);
	}
	int awaken::__done()
	{
		ev_async_send(cosys::at(m_tid), &m_awaken);
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