/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <thread>
#include <atomic>
#include <coev/coev.h>

using namespace coev;

co_channel<int> ch;

std::atomic<int> total = 0;
awaitable<void> go()
{
	LOG_DBG("go\n");
	int x = 0;
	for (int i = 0; i < 10000; i++)
	{
		x++;
		ch.set(x);
		x = co_await ch.get();
		LOG_DBG("x=%d\n", x);
	}
	total += x;
	LOG_DBG("total:%d\n", total.load());
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);
	runnable::instance().start(2, go).join();
	return 0;
}