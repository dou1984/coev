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
#include "async.h"
#include "co_event.h"
#include "co_list.h"
namespace coev
{
	class co_deliver
	{
	public:
		co_deliver();
		virtual ~co_deliver();

		static bool resume(async &waiter);
		static bool resume(co_event *ev);
		int id() const { return m_tid; }
		void stop();

	protected:
		int call_resume(co_event *ev);

	private:
		ev_async m_deliver;
		std::mutex m_lock;
		async m_waiter;
		uint64_t m_tid = 0;
		struct ev_loop *m_loop = nullptr;
		static void cb_async(struct ev_loop *loop, ev_async *w, int revents);

		void __init_local();
		void __fini_local();
		void __init();
		void __fini();
		int __resume_ev();
		int __done();
	};

}
