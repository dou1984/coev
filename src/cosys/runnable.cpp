/*
 *	cosys - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <sys/signal.h>
#include <unistd.h>
#include "cosys.h"
#include "runnable.h"

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
	void runnable::__add(const std::function<void()> &f)
	{
		m_list.emplace_back(
			[=]()
			{
				f();
				cosys::start();
			});
	}
	void runnable::join()
	{
		for (auto &it : m_list)
			it.join();
	}
	void runnable::detach()
	{
		for (auto &it : m_list)
			it.detach();
	}
}