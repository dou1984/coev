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
#include <atomic>

using namespace coev;

// int g_total = 0;
std::atomic_int g_total{0};
comutex g_mutex;
// std::mutex g_mutex;
std::atomic_int g_count{0};
waitgroup wg;
awaiter test_go()
{
	wg.add(1);
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
	wg.done();
	co_return 0;
}

awaiter test_wait()
{
	co_await wg.wait();

	co_await sleep_for(10);
	co_return 0;
}
int main()
{
	// set_log_level(LOG_LEVEL_CORE);
	running::instance().add(100, test_go).add(test_wait).join();

	return 0;
}