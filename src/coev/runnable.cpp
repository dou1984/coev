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
#include "finally.h"
#include "runnable.h"
#include "co_task.h"
#include "co_deliver.h"
#include "sleep_for.h"
#include "io_terminal.h"

namespace coev
{
	guard::co_async g_exception;
	std::atomic_int g_loop_count = 0;
	guard::co_async g_exit;

	runnable::runnable() noexcept
	{
		ignore_signal(SIGPIPE);
	}
	void runnable::end(const std::function<void()> &_cleanup) noexcept
	{
		bool terminated = false;
		co_start << [](auto &terminated) -> awaitable<void>
		{
			co_await g_exit.suspend(
				[]()
				{ return true; }, []() {});
			if (!terminated)
			{
				cosys::stop();
			}
		}(terminated);

		for (auto &it : m_list)
		{
			it.detach();
		}
		intercept_singal();
		cosys::start();
		terminated = true;
		_cleanup();
		LOG_CORE("main runnable is stopped by signal");
		while (g_exception.deliver())
		{
			LOG_CORE("deliver exit message");
		}
		while (g_loop_count != 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		m_list.clear();
	}
	runnable &runnable::start(const func &_f) noexcept
	{
		__add(_f);
		return *this;
	}
	runnable &runnable::start(int count, const func &_f) noexcept
	{
		for (int i = 0; i < count; i++)
		{
			__add(_f);
		}
		return *this;
	}
	void runnable::__add(const func &_f) noexcept
	{
		m_list.emplace_back(
			[=]()
			{
				ignore_signal(SIGPIPE);
				co_start << [](auto &_f) -> awaitable<void>
				{
					auto tid = gtid();
					co_await _f();
					cosys::stop();
					LOG_CORE("cosys stop tid:%ld", tid);
				}(_f);
				co_start << []() -> awaitable<void>
				{
					auto tid = gtid();
					co_await g_exception.suspend(
						[]()
						{ return true; }, []() {});
					cosys::stop();
					LOG_CORE("cosys stop tid:%ld", tid);
				}();
				++g_loop_count;
				finally(
					{
						if (--g_loop_count == 0)
						{
							g_exit.deliver(0);
						}
					});
				finally(co_start.destroy());
				cosys::start();
			});
	}
}