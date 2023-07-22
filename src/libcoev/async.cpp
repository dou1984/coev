/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <array>
#include "log.h"
#include "async.h"
#include "loop.h"

namespace coev
{
	void async::cb_async(struct ev_loop *loop, ev_async *w, int revents)
	{
		if (revents & EV_ASYNC)
		{
			async *_this = (async *)w->data;
			assert(_this != nullptr);
			_this->__resume();
		}
	}
	async::async() : async(loop::data(), ttag())
	{
	}
	async::async(struct ev_loop *__loop, uint64_t __tag)
	{
		m_Async.data = this;
		ev_async_init(&m_Async, async::cb_async);
		ev_async_start(__loop, &m_Async);
		m_tag = __tag;
	}
	async::~async()
	{
		ev_async_stop(loop::at(m_tag), &m_Async);
	}
	int async::resume()
	{
		ev_async_send(loop::at(m_tag), &m_Async);
		return 0;
	}
	int async::resume_event(event *ev)
	{
		std::lock_guard<decltype(m_lock)> _(m_lock);
		EVRecv::push_back(ev);
		return resume();
	}
	int async::__resume()
	{
		EVRecv _list;
		{
			std::lock_guard<decltype(m_lock)> _(m_lock);
			EVRecv::moveto(&_list);
		}
		while (_list.resume())
		{
		}
		return 0;
	}
}