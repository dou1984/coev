/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "timer.h"
#include "libev.h"

namespace coev
{
	void timer::cb_timer(struct ev_loop *loop, struct ev_timer *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		timer *_this = (timer *)(w->data);
		assert(_this != NULL);
		coev::resume(_this->m_trigger);
	}
	timer::timer(ev_tstamp itimer, ev_tstamp rtimer)
	{
		m_data.data = this;
		ev_timer_init(&m_data, timer::cb_timer, itimer, rtimer);
		m_tid = gtid();
	}
	timer::~timer()
	{
		if (ev_is_active(&m_data))
			ev_timer_stop(libev::at(m_tid), &m_data);
	}
	int timer::stop()
	{
		if (ev_is_active(&m_data))
			ev_timer_stop(libev::at(m_tid), &m_data);
		return 0;
	}
	int timer::active()
	{
		if (!ev_is_active(&m_data))
			ev_timer_start(libev::at(m_tid), &m_data);
		return 0;
	}
	bool timer::is_active()
	{
		return ev_is_active(&m_data);
	}
	ev_tstamp timer::remaining()
	{
		return ev_timer_remaining(libev::at(m_tid), &m_data);
	}
}