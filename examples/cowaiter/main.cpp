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
#include <coev.h>

using namespace coev;

cowaiter g_waiter;

awaiter<int> test_co()
{
	g_waiter.add();
	LOG_FATAL("add");
	co_await sleep_for(3);
	LOG_FATAL("done");
	g_waiter.done();
	co_return 0;
}
awaiter<int> test_wait()
{

	for (int i = 0; i < 10; i++)
	{
		test_co();
	}

	co_await wait_for_all(g_waiter.wait(), g_waiter.wait(), g_waiter.wait());

	LOG_FATAL("wait");
	co_return 0;
}
int main()
{

	routine::instance().add(test_wait);
	routine::instance().join();

	return 0;
}