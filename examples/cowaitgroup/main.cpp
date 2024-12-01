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
#include "../cosys/coloop.h"

using namespace coev;

waitgroup g_waiter;

awaitable<void> test_co()
{
	g_waiter.add();
	LOG_FATAL("add\n");

	g_waiter.done();

	co_return;
}
awaitable<int> test_wait()
{
	for (int i = 0; i < 2; i++)
	{
		test_co();
	}
	co_await g_waiter.wait();
	LOG_FATAL("suspend\n");
	co_return 0;
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);
	running::instance().add(test_wait).join();

	return 0;
}