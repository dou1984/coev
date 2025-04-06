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
#include <coev/coev.h>

using namespace coev;

guard::co_mutex g_mutex;
std::atomic_int g_total = {0};
awaitable<void> test_go()
{
	auto tid = gtid();
	auto now = std::chrono::system_clock::now();
	for (int i = 0; i < 10000; i++)
	{
		co_await g_mutex.lock();
		++g_total;
		g_mutex.unlock();
	}
	auto r = std::chrono::system_clock::now() - now;
	co_await sleep_for(1);
	defer _([&]()
			{ LOG_DBG("test_go end tid %ld total:%d \n", tid, g_total.load()); });
	co_return;
}

guard::co_mutex g_lock;
awaitable<void> test_lock()
{
	LOG_DBG("test_lock begin\n");
	co_await g_lock.lock();
	LOG_DBG("test_lock locked\n");

	co_await sleep_for(1);
	LOG_DBG("co_deliver\n");
	g_lock.unlock();
	LOG_DBG("test_lock end\n");
}
int main()
{
	set_log_level(LOG_LEVEL_DEBUG);
	runnable::instance()
		.add(100, test_go)
		// .add(4, test_lock)
		.join();

	return 0;
}