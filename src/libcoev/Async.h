/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <ev.h>
#include <mutex>
#include "Event.h"
#include "EventChain.h"

namespace coev
{
	class Async : public EVRecv
	{
	public:
		Async();
		Async(struct ev_loop *, int);
		~Async();

		int resume_event(Event *ev);

	private:
		ev_async m_Async;
		std::mutex m_lock;
		uint32_t m_tag = 0;
		static void cb_async(struct ev_loop *loop, ev_async *w, int revents);
		int __resume();
		int resume();
	};

}
