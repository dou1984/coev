/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <thread>
#include <atomic>
#include <chrono>
#include <coev/coev.h>
#include <cosys/cosys.h>

using namespace coev;

co_waitgroup g_waiter;

awaitable<int> test_co()
{
	g_waiter.add();
	LOG_FATAL("add\n");
	co_await sleep_for(1);
	LOG_FATAL("done\n");
	g_waiter.done();

	co_return 0;
}
awaitable<int> test_wait()
{
	for (int i = 0; i < 2; i++)
	{
		test_co();
	}
	co_await g_waiter.wait();
	LOG_FATAL("wait\n");
	co_return 0;
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);
	running::instance().add(test_wait).join();

	return 0;
}