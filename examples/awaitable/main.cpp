/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev/coev.h>

#define g singleton<coev::co_waitgroup>::instance()

using namespace coev;

async g_test;

awaitable<int> co_awaitable()
{
	g.add();

	defer([]()
		  { g.done(); });

	LOG_DBG("awaitable begin\n");
	co_await g_test.suspend();
	LOG_DBG("awaitable end\n");

	co_return 0;
}
awaitable<int> co_resume()
{
	g.add();
	defer _([]()
			{ g.done(); });

	LOG_DBG("resume begin\n");
	co_await sleep_for(5);
	LOG_DBG("resume end\n");
	g_test.resume();
	co_return 0;
}

awaitable<int> co_awaitable_sleep()
{
	g.add();
	defer _([]()
			{ g.done(); });

	LOG_DBG("awaitable sleep\n");
	co_await sleep_for(1);
	LOG_DBG("awaitable wakeup\n");
	co_await g_test.suspend();
	LOG_DBG("co_awaitable_sleep end\n");
	co_return 0;
}
awaitable<int> co_awaitable_resume()
{
	g.add();
	defer _([]()
			{ g.done(); });

	co_await sleep_for(2);
	LOG_DBG("co_awaitable_resume wakeup\n");
	g_test.resume();
	co_return 0;
}

awaitable<int, float, int, int, double> co_awaiter_tuple()
{
	g.add();
	defer _([]()
			{ g.done(); });

	co_return {10, 20.0, 345, 6, 78.0};
}

awaitable<void> co_awaiter_tuple_ex()
{
	g.add();
	defer _([]()
			{ g.done(); });

	auto [a, b, c, d, e] = co_await co_awaiter_tuple();
	LOG_DBG("%d %f %d %d %f\n", a, b, c, d, e);
}
awaitable<void> co_await_destroy()
{
	g.add();
	defer _([]()
			{ g.done(); });

	auto x = []() -> awaitable<void>
	{
		defer _([]()
				{ LOG_DBG("defer called !!!\n"); });
		co_await sleep_for(5);
	}();
	LOG_DBG("co_await_destroy sleep begin\n");
	co_await sleep_for(1);
	LOG_DBG("co_await_destroy destroy\n");
	x.destroy();
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);

	auto &run = runnable::instance() << []() -> awaitable<void>
	{
		co_await wait_for_all(co_awaitable(), co_resume());
		LOG_DBG("co_awaitable co_resume finish\n");
	} << []() -> awaitable<void>
	{
		co_await wait_for_all(co_awaitable_sleep(), co_awaitable_resume());
		LOG_DBG("co_awaitable_sleep co_awaitable_resume finish\n");
	} << []() -> awaitable<void>
	{
		co_await co_awaiter_tuple_ex();
	} << co_await_destroy
	  << []() -> awaitable<void>
	{
		co_await g.wait();		
		LOG_DBG("main finish\n");
	};
	run.join();
	return 0;
}