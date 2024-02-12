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

awaiter test_co()
{
	g_waiter.add();
	LOG_FATAL("add\n");
	co_await sleep_for(3);
	LOG_FATAL("done\n");
	g_waiter.done();
	co_return 0;
}
awaiter test_wait()
{

	for (int i = 0; i < 10; i++)
	{
		test_co();
	}
	co_await g_waiter.wait();
	LOG_FATAL("wait\n");
	co_return 0;
}
int main()
{

	running::instance().add(test_wait).join();

	return 0;
}