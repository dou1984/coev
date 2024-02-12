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
#include <atomic>
#include "../cosys/coloop.h"

using namespace coev;

comutex g_mutex;
std::atomic_int g_total{0};
std::atomic_int g_count{0};

awaiter test_go()
{
	co_await sleep_for(1);
	auto now = std::chrono::system_clock::now();
	for (int i = 0; i < 20; i++)
	{
		co_await g_mutex.lock();
		// g_mutex.lock();
		g_total += 1;
		// g_mutex.unlock();
		co_await g_mutex.unlock();
		co_await usleep_for(1);
	}

	co_await sleep_for(1);
	auto r = std::chrono::system_clock::now() - now;
	auto _count = g_count++;
	LOG_DBG("%d %d %ld\n", _count, g_total.load(), r.count());
	co_return 0;
}

int main()
{
	// set_log_level(LOG_LEVEL_CORE);
	running::instance()
		.add(100, test_go)
		.join();

	return 0;
}