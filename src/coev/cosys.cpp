/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <atomic>
#include <list>
#include <mutex>
#include <algorithm>
#include <utility>
#include "cosys.h"
#include "local.h"
#include "co_deliver.h"
#include "co_task.h"
#include "io_terminal.h"
#include "defer.h"

#define g_loop local<__ev_loop>::instance()
namespace coev
{
	class __ev_loop
	{
		struct ev_loop *m_loop = nullptr;

	public:
		__ev_loop()
		{
			m_loop = ev_loop_new();
			LOG_CORE("ev_loop_new %p", m_loop);
		}
		~__ev_loop()
		{
			if (m_loop)
			{
				co_start.destroy();
				auto _loop = std::exchange(m_loop, nullptr);
				ev_loop_destroy(_loop);
				LOG_CORE("ev_loop_destroy %p", _loop);
			}
		}
		struct ev_loop *get() const
		{
			return m_loop;
		}
	};

	void cosys::start()
	{
		auto _loop = g_loop.get();
		auto id = local<co_deliver>::instance().id();
		ev_run(_loop, 0);
		LOG_CORE("loop run stop %p", _loop);
	}
	void cosys::stop()
	{
		local<co_deliver>::instance().stop();
		auto tid = gtid();
		auto _loop = g_loop.get();
		ev_break(_loop, EVBREAK_ALL);
		LOG_CORE("loop stop %p", _loop);
	}
	struct ev_loop *cosys::data()
	{
		return g_loop.get();
	}
}