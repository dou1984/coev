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

	LOG_DBG("awaitable begin\n");
	co_await g_test.suspend();
	LOG_DBG("awaitable end\n");

	g.done();
	co_return 0;
}
awaitable<int> co_resume()
{
	g.add();
	LOG_DBG("resume begin\n");
	co_await sleep_for(5);
	LOG_DBG("resume end\n");
	g_test.resume();

	g.done();
	co_return 0;
}

awaitable<int> co_awaitable_sleep()
{
	g.add();

	LOG_DBG("awaitable sleep\n");
	co_await sleep_for(1);
	LOG_DBG("awaitable wakeup\n");
	co_await g_test.suspend();
	LOG_DBG("co_awaitable_sleep end\n");
	g.done();
	co_return 0;
}
awaitable<int> co_awaitable_resume()
{
	g.add();

	co_await sleep_for(2);
	LOG_DBG("co_awaitable_resume wakeup\n");
	g_test.resume();
	g.done();
	co_return 0;
}

awaitable<int, float, int, int, double> co_awaiter_tuple()
{
	g.add();
	g.done();

	co_return {10, 20.0, 345, 6, 78.0};
}

awaitable<void> co_awaiter_tuple_ex()
{
	g.add();
	g.done();

	auto [a, b, c, d, e] = co_await co_awaiter_tuple();
	LOG_DBG("%d %f %d %d %f\n", a, b, c, d, e);
}
awaitable<void> co_await_destroy()
{
	g.add();

	auto x = []() -> awaitable<void>
	{
		co_await sleep_for(5);
		LOG_DBG("error called !!!\n");
	}();
	LOG_DBG("co_await_destroy sleep begin\n");
	co_await sleep_for(1);
	LOG_DBG("co_await_destroy destroy\n");
	x.destroy();

	g.done();
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);

	runnable::instance()
		.start(
			[]() -> awaitable<void>
			{
				co_await wait_for_all(co_awaitable(), co_resume());
				LOG_DBG("co_awaitable co_resume finish\n");
			})
		.start(
			[]() -> awaitable<void>
			{
				co_await wait_for_all(co_awaitable_sleep(), co_awaitable_resume());
				LOG_DBG("co_awaitable_sleep co_awaitable_resume finish\n");
			})
		.start(
			[]() -> awaitable<void>
			{ co_await co_awaiter_tuple_ex(); })
		.start(co_await_destroy)
		.start(
			[]() -> awaitable<void>
			{
				co_await g.wait();
				LOG_DBG("main finish\n");
			})
		.join();
	return 0;
}