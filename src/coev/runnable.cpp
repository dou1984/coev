/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <sys/signal.h>
#include <memory>
#include <unistd.h>
#include "cosys.h"
#include "runnable.h"
#include "co_task.h"
#include "sleep_for.h"
#include "co_deliver.h"

namespace coev
{
	void ingore_signal(int sign)
	{
		struct sigaction s = {};
		s.sa_handler = SIG_IGN;
		s.sa_flags = 0;
		sigaction(sign, &s, NULL);
	}
	runnable::runnable()
	{
		ingore_signal(SIGPIPE);
	}

	void runnable::join()
	{
		for (auto &it : m_list)
		{
			it.join();
		}
	}
	void runnable::detach()
	{
		for (auto &it : m_list)
		{
			it.detach();
		}
	}
	runnable &runnable::operator<<(const func &_f)
	{
		__add(_f);
		return *this;
	}
	runnable &runnable::add(const func &_f)
	{
		__add(_f);
		return *this;
	}
	runnable &runnable::add(int count, const func &_f)
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
				coev::guard::async end;
				co_start << [&end]() -> awaitable<void>
				{
					co_await end.suspend([]()
										 { return true; }, []() {});
					local<co_deliver>::instance().stop();
					LOG_DBG("tid: %ld before finished\n", gtid());
				}();
				co_start << [&end, _f]() -> awaitable<void>
				{
					co_await _f();
					while (co_deliver::resume(local<async>::instance()))
					{
					}
					end.deliver([]() {});
				}();
				cosys::start();
				LOG_DBG("tid: %ld end\n", gtid());
			});
	}

}