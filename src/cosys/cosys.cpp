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
#include "evpipe.h"

#define g_loop local<unique<__ev_loop>>::instance()

namespace coev
{
	struct __ev_loop
	{
		struct ev_loop *m_loop = nullptr;
		__ev_loop()
		{
			m_loop = ev_loop_new();
		}
		~__ev_loop()
		{
			ev_loop_destroy(m_loop);
		}
	};

	void cosys::start()
	{
		auto _loop = g_loop->m_loop;
		evpipe __pipe;
		ev_run(_loop, 0);
	}
	void cosys::stop()
	{
		auto _loop = g_loop->m_loop;
		ev_loop_destroy(_loop);
	}

	struct ev_loop *cosys::data()
	{
		return g_loop->m_loop;
	}
	struct ev_loop *cosys::at(uint64_t _tid)
	{
		return _tid == gtid() ? g_loop->m_loop : g_loop.at(_tid)->m_loop;
	}
}