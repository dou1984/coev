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
#include "coev/coev.h"

using namespace coev;

guard::co_mutex g_mutex;
std::atomic_int g_total = {0};
awaitable<void> test_go()
{
	auto tid = gtid();
	auto now = std::chrono::system_clock::now();
	for (int i = 0; i < 100; i++)
	{
		co_await g_mutex.lock();
		++g_total;
		g_mutex.unlock();
	}
	auto r = std::chrono::system_clock::now() - now;
	co_await sleep_for(1);
	LOG_DBG("test_go tid %ld total:%d \n", tid, g_total.load());
	co_return;
}

int main()
{
	set_log_level(LOG_LEVEL_DEBUG);
	runnable::instance()
		.start(100, test_go)
		// .start(4, test_lock)
		.join();

	return 0;
}