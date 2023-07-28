/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <ev.h>
#include "../coev.h"

namespace coev
{
	struct timer final : EVTimer
	{
		ev_timer m_data;
		uint64_t m_tag = 0;

		timer(ev_tstamp itimer, ev_tstamp rtimer);
		virtual ~timer();
		int active();
		int stop();
		bool is_active();
		ev_tstamp remaining();

		static void cb_timer(struct ev_loop *loop, struct ev_timer *w, int revents);		
	};
}