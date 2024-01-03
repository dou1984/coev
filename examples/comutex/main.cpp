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

awaiter test_go()
{
	int _total = 0;
	auto now = std::chrono::system_clock::now();
	for (int i = 0; i < 100000; i++)
	{
		co_await g_mutex.lock();
		// g_mutex.lock();
		g_total += 1;
		_total += 1;
		// g_mutex.unlock();
		co_await g_mutex.unlock();
	}
	auto r = std::chrono::system_clock::now() - now;

	LOG_DBG("%d %d %ld \n", g_total.load(), _total, r.count());
	co_return 0;
}

int main()
{
	// set_log_level(LOG_LEVEL_CORE);
	running::instance().add(8, test_go).join();

	return 0;
}