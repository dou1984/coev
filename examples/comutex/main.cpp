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
#include <cosys/cosys.h>

using namespace coev;

thread_safe::comutex g_mutex;
std::atomic_int g_total{0};
std::atomic_int g_count{0};

awaitable<int> test_go()
{
	co_await sleep_for(1);
	auto now = std::chrono::system_clock::now();
	for (int i = 0; i < 100; i++)
	{
		co_await g_mutex.lock();		
		g_total += 1;		
		g_mutex.unlock();
		co_await usleep_for(1);
	}

	co_await sleep_for(10);
	auto r = std::chrono::system_clock::now() - now;
	auto _count = g_count++;
	LOG_DBG("%d %d %ld\n", _count, g_total.load(), r.count());
	co_return 0;
}

thread_safe::comutex g_lock;
awaitable<void> test_lock()
{
	LOG_DBG("test_lock begin\n");
	co_await g_lock.lock();
	LOG_DBG("test_lock locked\n");

	co_await sleep_for(1);
	LOG_DBG("awaken\n");
	g_lock.unlock();

	
	LOG_DBG("test_lock end\n");

}
int main()
{
	set_log_level(LOG_LEVEL_CORE);
	running::instance()
		.add(100, test_go)
		.add(4, test_lock)
		.join();

	return 0;
}