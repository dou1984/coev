/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
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

	for (int i = 0; i < 200; i++)
	{
		co_await g_mutex.lock();
		++g_total;
		g_mutex.unlock();
	}
	LOG_DBG("tid:%ld:%ld total:%d \n", tid, gtid(), g_total.load());
	co_await sleep_for(1);

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