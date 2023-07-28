/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <sys/signal.h>
#include <unistd.h>
#include "loop.h"
#include "routine.h"

namespace coev
{
	void ingore_signal(int sign)
	{
		struct sigaction s = {};
		s.sa_handler = SIG_IGN;
		s.sa_flags = 0;
		sigaction(sign, &s, NULL);
	}
	routine::routine()
	{
		ingore_signal(SIGPIPE);
	}
	void routine::__add(const std::function<void()> &f)
	{
		m_list.emplace_back(
			[=]()
			{
				f();
				loop::start();
			});
	}
	void routine::join()
	{
		for (auto &it : m_list)
			it.join();
	}
}