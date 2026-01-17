/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <sys/signal.h>
#include <memory>
#include <unistd.h>
#include <thread>
#include <atomic>
#include "cosys.h"
#include "defer.h"
#include "runnable.h"
#include "co_task.h"
#include "co_deliver.h"
#include "sleep_for.h"
#include "io_terminal.h"

namespace coev
{
	guard::async g_exception;
	std::atomic_int g_loop_count = 0;
	runnable::runnable()
	{
		ingore_signal(SIGPIPE);
	}

	void runnable::wait()
	{
		for (auto &it : m_list)
		{
			it.join();
		}
	}
	void runnable::endless(const std::function<void()> &_cleanup)
	{
		for (auto &it : m_list)
		{
			it.detach();
		}
		intercept_singal();
		cosys::start();
		_cleanup();
		LOG_CORE("main runnable is stopped by signal");
		while (g_exception.deliver(0))
		{
			LOG_CORE("deliver exit message");
		}
		while (g_loop_count != 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}
	runnable &runnable::start(const func &_f)
	{
		__add(_f);
		return *this;
	}
	runnable &runnable::start(int count, const func &_f)
	{
		for (int i = 0; i < count; i++)
		{
			__add(_f);
		}
		return *this;
	}
	void runnable::__add(const func &_f)
	{
		m_list.emplace_back(
			[=]()
			{
				co_start << [=]() -> awaitable<void>
				{
					co_await _f();
					cosys::stop();
					LOG_CORE("cosys stop");
				}();
				co_start << []() -> awaitable<void>
				{
					co_await g_exception.suspend(
						[]()
						{ return true; }, []() {});
					cosys::stop();
					LOG_CORE("cosys stop");
				}();
				++g_loop_count;
				defer(--g_loop_count);
				cosys::start();
			});
	}

}