/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <atomic>
#include <list>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include "loop.h"
#include "async.h"

#define g_loop threadlocal<__this_ev_loop>::instance()

namespace coev
{

	struct __ev_loop;
	static std::unordered_map<uint64_t, struct __ev_loop *> all_loops;
	static std::mutex g_mutex;
	struct __ev_loop
	{
		uint64_t m_tag = 0;
		struct ev_loop *m_loop = nullptr;
		__ev_loop()
		{
			m_tag = ttag();
			m_loop = ev_loop_new();
			std::lock_guard<std::mutex> _(g_mutex);
			all_loops.emplace(m_tag, this);
		}
		~__ev_loop()
		{
			ev_loop_destroy(m_loop);
			std::lock_guard<std::mutex> _(g_mutex);
			all_loops.erase(m_tag);
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
	struct ev_loop *loop::at(uint64_t _tag)
	{
		if (data() == nullptr)
		{
			LOG_ERR("error g_loop.m_loop is nullptr\n");
		}
		std::lock_guard<std::mutex> _(g_mutex);
		if (auto it = all_loops.find(_tag); it != all_loops.end())
			return it->second->m_loop;
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
			__this_ev_loop *__loop = nullptr;
			{
				std::lock_guard<std::mutex> _(g_mutex);
				if (auto it = all_loops.find(ev->m_tag); it != all_loops.end())
				{
					__loop = static_cast<__this_ev_loop *>(it->second);
				}
			}
			if (__loop)
			{
				__loop->async::resume_event(ev);
			}
		}
	}
}