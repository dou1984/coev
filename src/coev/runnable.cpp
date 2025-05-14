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
				guard::async _end;
				co_start << [&_end]() -> awaitable<void>
				{
					co_await _end.suspend([]()
										  { return true; }, []() {});
					local<co_deliver>::instance().stop();
				}();
				co_start << [=, &_end]() -> awaitable<void>
				{
					co_await _f();
					_end.deliver();
				}();
				cosys::start();
			});
	}

}