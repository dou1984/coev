/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <array>
#include <atomic>
#include <list>
#include <mutex>
#include "threadLocal.h"
#include "Loop.h"
#include "Log.h"
#include "Async.h"

#define g_loop ThreadLocal<__this_ev_loop>::instance()

namespace coev
{
	struct __ev_loop;
	static std::array<struct __ev_loop *, max_ev_loop> all_loops = {nullptr};
	static std::atomic<uint32_t> g_index{0};

	struct __ev_loop
	{
		uint32_t m_tag = 0;
		struct ev_loop *m_loop = nullptr;
		__ev_loop()
		{
			m_tag = g_index++;
			all_loops[m_tag] = this;
			m_loop = ev_loop_new();
		}
		~__ev_loop()
		{
			ev_loop_destroy(m_loop);
			all_loops[m_tag] = nullptr;
		}
		uint32_t tag()
		{
			return __ev_loop::m_tag;
		}
	};
	struct __this_ev_loop : __ev_loop, Async
	{
		__this_ev_loop() : __ev_loop(), Async(__ev_loop::m_loop, __ev_loop::m_tag)
		{
		}
	};

	void Loop::start()
	{
		ev_run(g_loop.m_loop, 0);
	}
	struct ev_loop *Loop::data()
	{
		return g_loop.m_loop;
	}
	struct ev_loop *Loop::at(uint32_t _tag)
	{
		if (_tag < max_ev_loop)
			return all_loops[_tag]->m_loop;
		return nullptr;
	}
	uint32_t Loop::tag()
	{
		return g_loop.tag();
	}
	void Loop::resume(Event *ev)
	{
		if (ev->m_tag == g_loop.tag())
		{
			ev->resume_ex();
		}
		else
		{
			auto __loop = static_cast<__this_ev_loop *>(all_loops[ev->m_tag]);
			assert(__loop);
			__loop->Async::resume_event(ev);
		}
	}
}