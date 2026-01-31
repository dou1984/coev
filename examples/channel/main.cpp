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
awaitable<void> task_01()
{
	LOG_DBG("task_01");
	int x = 0;
	for (int i = 0; i < 1000; i++)
	{
		x++;
		ch.set(x);

		co_await ch.get(x);
		LOG_DBG("x=%d", x);
	}
	total += x;
	LOG_DBG("total:%d", total.load());
}

awaitable<void> task_02()
{
	LOG_DBG("task_02");
	int x = 0;
	for (int i = 0; i < 1000; i++)
	{
		x++;
		ch.set(x);
		co_await ch.get(x);
		LOG_DBG("x=%d", x);
	}
	total += x;
	LOG_DBG("total:%d", total.load());
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);
	runnable::instance()
		.start(2, task_01)
		.start(2, task_02)
		.wait();
	return 0;
}