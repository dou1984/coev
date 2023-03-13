/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <ev.h>
#include "Event.h"
#include "EventSet.h"

namespace coev
{
	struct Timer final : EVTimer
	{
		ev_timer m_data;
		uint32_t m_tag = 0;

		Timer(ev_tstamp itimer, ev_tstamp rtimer);
		virtual ~Timer();
		int active();
		int stop();
		bool is_active();
		ev_tstamp remaining();

		static void cb_timer(struct ev_loop *loop, struct ev_timer *w, int revents);		
	};
}