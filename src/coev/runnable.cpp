/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <sys/signal.h>
#include <unistd.h>
#include "cosys.h"
#include "runnable.h"
#include "co_task.h"

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
			[_f, this]()
			{
				co_task _task;
				_task << [_f, this]() -> awaitable<void>
				{
					co_start << _f();
					co_await co_start.wait_all();
					__deliver_resume();
				}();
				cosys::start();
			});
	}
	void runnable::__deliver_resume()
	{
		while (co_deliver::resume(local<async>::instance()))
		{
		}
		while (co_deliver::resume(local<co_list>::instance()))
		{
		}
	}
}