/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <sys/signal.h>
#include <memory>
#include <unistd.h>
#include "cosys.h"
#include "runnable.h"
#include "co_task.h"
#include "co_deliver.h"
#include "sleep_for.h"

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
				co_start << [=]() -> awaitable<void>
				{
					co_await _f();
					cosys::stop();
				}();
				cosys::start();
			});
	}

}