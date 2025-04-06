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

using namespace coev;

co_waitgroup g_waiter;

awaitable<int> test_co()
{
	g_waiter.add();
	LOG_INFO("add\n");
	co_await sleep_for(1);
	LOG_INFO("done\n");
	g_waiter.done();

	co_return 0;
}
awaitable<void> test_wait()
{
	for (int i = 0; i < 2; i++)
	{
		co_start << test_co();
	}
	co_await g_waiter.wait();
	LOG_INFO("wait\n");
	co_return;
}
int main()
{
	set_log_level(LOG_LEVEL_DEBUG);
	auto &run = runnable::instance() << test_wait;
	run.join();

	return 0;
}