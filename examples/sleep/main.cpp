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
Awaiter<int> co_sleep()
{
	auto now = std::chrono::system_clock::now();
	for (int i = 0; i < 3; i++)
	{
		int t = 2;
		co_await sleep_for(t);
		LOG_DBG("sleep_for %d\n", t);
	}
	auto r = std::chrono::system_clock::now() - now;
	LOG_DBG("co_sleep %ld\n", r.count());
}

Awaiter<int> co_timer()
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
int main()
{

	Routine r;
	r.add(
		[]()
		{
			co_sleep();
			co_timer();
		});
	r.join();
	return 0;
}