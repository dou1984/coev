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
#include "threadlocal.h"
#include "loop.h"
#include "log.h"
#include "async.h"
#include "tag.h"

#define g_loop threadlocal<__this_ev_loop>::instance()

namespace coev
{
	struct __ev_loop;
	static std::array<struct __ev_loop *, max_ev_loop> all_loops = {nullptr};

	struct __ev_loop
	{
		uint32_t m_tag = 0;
		struct ev_loop *m_loop = nullptr;
		__ev_loop()
		{
			m_tag = ttag();
			all_loops[m_tag] = this;
			m_loop = ev_loop_new();
		}
		~__ev_loop()
		{
			ev_loop_destroy(m_loop);
			all_loops[m_tag] = nullptr;
		}
	};
	struct __this_ev_loop : __ev_loop, async
	{
		__this_ev_loop() : __ev_loop(), async(__ev_loop::m_loop, __ev_loop::m_tag)
		{
		}
	};

	void loop::start()
	{
		ev_run(g_loop.m_loop, 0);
	}
	struct ev_loop *loop::data()
	{
		return g_loop.m_loop;
	}
	struct ev_loop *loop::at(uint32_t _tag)
	{
		if (_tag < max_ev_loop)
			return all_loops[_tag]->m_loop;
		return nullptr;
	}
	void loop::resume(event *ev)
	{
		if (ev->m_tag == ttag())
		{
			ev->resume();
		}
		else
		{
			auto __loop = static_cast<__this_ev_loop *>(all_loops[ev->m_tag]);
			__loop->async::resume_event(ev);
		}
	}
}