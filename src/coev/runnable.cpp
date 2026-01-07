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
#include "io_terminal.h"

namespace coev
{

	runnable::runnable()
	{
		ingore_signal(SIGPIPE);
		// intercept_singal();
	}

	runnable &runnable::join()
	{
		for (auto &it : m_list)
		{
			it.join();
		}
		return *this;
	}
	// runnable &runnable::wait_signal()
	// {
	// 	for (auto &it : m_list)
	// 	{
	// 		it.detach();
	// 	}
	// 	cosys::start();
	// 	return *this;
	// }
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