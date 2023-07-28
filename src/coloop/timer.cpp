/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "timer.h"
#include "loop.h"

namespace coev
{
	void timer::cb_timer(struct ev_loop *loop, struct ev_timer *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		timer *_this = (timer *)(w->data);
		assert(_this != NULL);
		_this->EVTimer::resume();
	}
	timer::timer(ev_tstamp itimer, ev_tstamp rtimer)
	{
		m_data.data = this;
		ev_timer_init(&m_data, timer::cb_timer, itimer, rtimer);
		m_tag = ttag();
	}
	timer::~timer()
	{
		if (ev_is_active(&m_data))
			ev_timer_stop(loop::at(m_tag), &m_data);
		assert(EVTimer::empty());
	}
	int timer::stop()
	{
		if (ev_is_active(&m_data))
			ev_timer_stop(loop::at(m_tag), &m_data);
		return 0;
	}
	int timer::active()
	{
		if (!ev_is_active(&m_data))
			ev_timer_start(loop::at(m_tag), &m_data);
		return 0;
	}
	bool timer::is_active()
	{
		return ev_is_active(&m_data);
	}
	ev_tstamp timer::remaining()
	{
		return ev_timer_remaining(loop::at(m_tag), &m_data);
	}
}