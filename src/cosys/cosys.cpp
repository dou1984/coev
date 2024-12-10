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
#include "cosys.h"
#include "awaken.h"

#define g_loop local<__this_ev_loop>::instance()

namespace coev
{
	struct __ev_loop;
	static std::unordered_map<uint64_t, struct __ev_loop *> all_loops;
	static std::mutex g_mutex;
	struct __ev_loop
	{
		uint64_t m_tid = 0;
		struct ev_loop *m_loop = nullptr;
		__ev_loop()
		{
			m_tid = gtid();
			m_loop = ev_loop_new();
			std::lock_guard<std::mutex> _(g_mutex);
			all_loops.emplace(m_tid, this);
		}
		~__ev_loop()
		{
			ev_loop_destroy(m_loop);
			std::lock_guard<std::mutex> _(g_mutex);
			all_loops.erase(m_tid);
		}
	};
	struct __this_ev_loop : __ev_loop, awaken
	{
		__this_ev_loop() : __ev_loop(), awaken(__ev_loop::m_loop, __ev_loop::m_tid)
		{
		}
	};

	void cosys::start()
	{
		ev_run(g_loop.m_loop, 0);
	}
	void cosys::stop()
	{
		std::lock_guard<std::mutex> _(g_mutex);
		for (auto &it : all_loops)
		{
			ev_loop_destroy(it.second->m_loop);
		}
	}
	struct ev_loop *cosys::data()
	{
		return g_loop.m_loop;
	}
	struct ev_loop *cosys::at(uint64_t _tid)
	{
		if (data() == nullptr)
		{
			LOG_ERR("error g_loop.m_loop is nullptr\n");
		}
		std::lock_guard<std::mutex> _(g_mutex);
		if (auto it = all_loops.find(_tid); it != all_loops.end())
			return it->second->m_loop;
		return nullptr;
	}
	void cosys::resume(event *ev)
	{
		if (ev->m_tid == gtid())
		{
			ev->resume();
		}
		else
		{
			__this_ev_loop *__loop = nullptr;
			{
				std::lock_guard<std::mutex> _(g_mutex);
				if (auto it = all_loops.find(ev->m_tid); it != all_loops.end())
				{
					__loop = static_cast<__this_ev_loop *>(it->second);
				}
			}
			if (__loop)
			{
				__loop->awaken::resume(ev);
			}
		}
	}
}