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
#include <coev/coev.h>

namespace coev
{
	class awaken
	{
	public:
		awaken();
		virtual ~awaken();

		int resume(co_event *ev);

	private:
		ev_async m_awaken;
		std::mutex m_lock;
		async m_waiter;
		uint64_t m_tid = 0;
		struct ev_loop *m_loop = nullptr;
		static void cb_async(struct ev_loop *loop, ev_async *w, int revents);
		void __init();
		int __resume();
		int __done();
	};

}
