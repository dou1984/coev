/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <stdio.h>
#include <coev.h>

using namespace coev;
awaiter<int> co_sleep()
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
	co_return 0;
}

awaiter<int> co_timer()
{
	Timer t(3.0, 3.0);
	t.active();
	while (true)
	{
		co_await wait_for<EVTimer>(t);
		LOG_DBG("timer %f\n", 3.0);
	}
	co_return 0;
}

awaiter<int> co_iterator(int t)
{
	if (t > 0)
	{
		co_await co_iterator(t - 1);
		co_await sleep_for(1);
		LOG_DBG("sleep for 1\n");
	}
	co_return 0;
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);
	Routine r;
	r.add(co_sleep);
	r.add(co_timer);
	r.add([]()
		  { co_iterator(10); });

	r.join();
	return 0;
}