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
#include <coev.h>

namespace coev
{
	class awaken 
	{
	public:
		awaken();
		awaken(struct ev_loop *, uint64_t);
		~awaken();

		int resume_event(event *ev);

	private:
		ev_async m_awaken;
		std::mutex m_lock;
		async m_trigger;
		uint64_t m_tid = 0;
		static void cb_async(struct ev_loop *loop, ev_async *w, int revents);
		int __resume();
		int resume();
	};

}
