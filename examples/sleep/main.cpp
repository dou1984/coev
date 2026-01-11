/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <stdio.h>
#include <coev/coev.h>

using namespace coev;
awaitable<void> test_sleep()
{
	auto now = std::chrono::system_clock::now();
	for (int i = 0; i < 3; i++)
	{
		int t = 2;
		co_await sleep_for(t);
		LOG_DBG("sleep_for %d\n", t);
	}
	auto r = std::chrono::system_clock::now() - now;
	LOG_DBG("co_sleep %ld.%09ld\n", r.count() / 1000000000, r.count() % 1000000000);
}

awaitable<void> test_timer()
{
	co_timer t(3.0, 3.0);
	t.active();
	for (auto i = 0; i < 10; ++i)
	{
		co_await t.suspend();
		LOG_DBG("co_timer %f\n", 3.0);
	}
}

awaitable<void> test_iterator(int t)
{
	if (t > 0)
	{
		co_await test_iterator(t - 1);
		co_await sleep_for(1);
		LOG_DBG("sleep for 1\n");
	}
}

int main()
{
	set_log_level(LOG_LEVEL_DEBUG);

	runnable::instance()
		.start(test_sleep)
		.start(test_timer)
		.start([]() -> awaitable<void>
			   { co_await test_iterator(10); })
		.wait();
	return 0;
}