/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <thread>
#include <atomic>
#include <coev/coev.h>
#include <coev/coev.h>

using namespace coev;

co_channel<int> ch;

std::atomic<int> total = 0;
awaitable<int> go()
{
	LOG_DBG("go\n");
	int x = 0;
	for (int i = 0; i < 100; i++)
	{
		x++;
		ch.set(x);
		x = co_await ch.move();
		LOG_DBG("x=%d\n", x);
	}
	total += x;
	LOG_DBG("total:%d\n", total.load());
	co_return x;
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);
	runnable::instance().add(2, go).join();
	return 0;
}