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
#include <coloop.h>

using namespace coev;

int g_total = 0;
comutex g_mutex;

awaiter test_go()
{
	auto now = std::chrono::system_clock::now();
	for (int i = 0; i < 100000; i++)
	{
		co_await g_mutex.lock();
		g_total += 1;
		co_await g_mutex.unlock();
	}
	auto r = std::chrono::system_clock::now() - now;

	printf("%d %ld\n", g_total, r.count());
	co_return 0;
}

int main()
{
	running::instance().add(8, test_go).join();

	return 0;
}