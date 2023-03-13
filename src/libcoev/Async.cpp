/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <array>
#include "Log.h"
#include "Async.h"
#include "Loop.h"

namespace coev
{
	void Async::cb_async(struct ev_loop *loop, ev_async *w, int revents)
	{
		if (revents & EV_ASYNC)
		{
			Async *_this = (Async *)w->data;
			assert(_this != nullptr);
			_this->__resume();
		}
	}
	Async::Async() : Async(Loop::data(), Loop::tag())
	{
	}
	Async::Async(struct ev_loop *__loop, int __tag)
	{
		m_Async.data = this;
		ev_async_init(&m_Async, Async::cb_async);
		ev_async_start(__loop, &m_Async);
		m_tag = __tag;
	}
	Async::~Async()
	{
		ev_async_stop(Loop::at(m_tag), &m_Async);
	}
	int Async::resume()
	{
		ev_async_send(Loop::at(m_tag), &m_Async);
		return 0;
	}
	int Async::resume_event(Event *ev)
	{
		std::lock_guard<decltype(m_lock)> _(m_lock);
		this->EVRecv::push_back(ev);
		return resume();
	}
	int Async::__resume()
	{
		EVRecv _list;
		{
			std::lock_guard<decltype(m_lock)> _(m_lock);
			this->EVRecv::moveto(&_list);
		}
		while (_list.resume_ex())
		{
		}
		return 0;
	}
}