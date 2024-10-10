/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "../cosys/coloop.h"

using namespace coev;

async g_test;

awaitable<int> co_awaitable()
{
	LOG_DBG("awaitable begin\n");
	co_await wait_for(g_test);
	LOG_DBG("awaitable end\n");
	co_return 0;
}
awaitable<int> co_resume()
{
	LOG_DBG("resume begin\n");
	// co_await sleep_for(5);
	LOG_DBG("resume end\n");
	notify(g_test);
	co_return 0;
}

awaitable<int> co_awaitable_sleep()
{
	auto ev = wait_for(g_test);
	LOG_DBG("awaitable sleep\n");
	// co_await sleep_for(1);
	LOG_DBG("awaitable wakeup\n");
	co_await ev;
	LOG_DBG("co_awaitable_sleep end\n");
	co_return 0;
}
awaitable<int> co_awaitable_resume()
{
	// co_await sleep_for(2);
	LOG_DBG("co_awaitable_resume wakeup\n");
	notify(g_test);
	co_return 0;
}
int main()
{
	set_log_level(LOG_LEVEL_DEBUG);

	running::instance()
		.add([]() -> awaitable<void>
			 { co_await wait_for_all(co_awaitable(), co_resume());
			 LOG_DBG("co_awaitable co_resume finish\n"); })
		.add([]() -> awaitable<void>
			 { co_await wait_for_all(co_awaitable_sleep(), co_awaitable_resume());
			  LOG_DBG("co_awaitable_sleep co_awaitable_resume finish\n"); })
		.join();
	return 0;
}