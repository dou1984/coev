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
#include "event.h"
#include "eventchain.h"

namespace coev
{
	class async : public EVRecv
	{
	public:
		async();
		async(struct ev_loop *, int);
		~async();

		int resume_event(event *ev);

	private:
		ev_async m_Async;
		std::mutex m_lock;
		uint32_t m_tag = 0;
		static void cb_async(struct ev_loop *loop, ev_async *w, int revents);
		int __resume();
		int resume();
	};

}
