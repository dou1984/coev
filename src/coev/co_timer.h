/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *
 */
#pragma once
#include <ev.h>
#include "async.h"

namespace coev
{
	class co_timer final
	{
		ev_timer m_data;
		uint64_t m_tid = 0;
		struct ev_loop *m_loop = nullptr;
		async m_waiter;
		static void cb_timer(struct ev_loop *loop, struct ev_timer *w, int revents);

	public:
		auto suspend() { return m_waiter.suspend(); }
		co_timer(ev_tstamp itimer, ev_tstamp rtimer);
		virtual ~co_timer();
		int active();
		int stop();
		bool is_active();
		ev_tstamp remaining();
	};
}