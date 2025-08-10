/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *
 */
#include "co_timer.h"
#include "cosys.h"
#include "local_resume.h"

namespace coev
{
	void co_timer::cb_timer(struct ev_loop *loop, struct ev_timer *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		co_timer *_this = (co_timer *)(w->data);
		assert(_this != NULL);
		LOG_CORE("co_timer::cb_timer %p\n", _this);
		_this->m_waiter.resume();
		local_resume();
	}
	co_timer::co_timer(ev_tstamp itimer, ev_tstamp rtimer)
	{
		m_data.data = this;
		ev_timer_init(&m_data, co_timer::cb_timer, itimer, rtimer);
		m_tid = gtid();
		m_loop = cosys::data();
	}
	co_timer::~co_timer()
	{
		if (ev_is_active(&m_data))
		{
			ev_timer_stop(m_loop, &m_data);
		}
	}
	int co_timer::stop()
	{
		if (ev_is_active(&m_data))
		{
			ev_timer_stop(m_loop, &m_data);
		}
		return 0;
	}
	int co_timer::active()
	{
		if (!ev_is_active(&m_data))
		{
			ev_timer_start(m_loop, &m_data);
		}
		return 0;
	}
	bool co_timer::is_active()
	{
		return ev_is_active(&m_data);
	}
	ev_tstamp co_timer::remaining()
	{
		return ev_timer_remaining(m_loop, &m_data);
	}
}