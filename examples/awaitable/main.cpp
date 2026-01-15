/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <coev/coev.h>

using namespace coev;

#define g singleton<co_waitgroup>::instance()

async g_waiter;
async g_t_waiter;

awaitable<int> co_awaitable()
{
	g.add();

	LOG_DBG("awaitable begin");
	co_await g_waiter.suspend();
	LOG_DBG("awaitable end");

	g.done();
	co_return 0;
}
awaitable<int> co_resume()
{
	g.add();
	LOG_DBG("resume begin");
	co_await sleep_for(5);
	LOG_DBG("resume end");
	g_waiter.resume();

	g.done();
	co_return 0;
}

awaitable<int> co_awaitable_sleep()
{
	g.add();

	LOG_DBG("awaitable sleep");
	co_await sleep_for(1);
	LOG_DBG("awaitable wakeup");
	co_await g_t_waiter.suspend();
	LOG_DBG("co_awaitable_sleep end");
	g.done();
	co_return 0;
}
awaitable<int> co_awaitable_resume()
{
	g.add();

	co_await sleep_for(2);
	LOG_DBG("co_awaitable_resume wakeup");
	g_t_waiter.resume();
	g.done();
	co_return 0;
}

awaitable<void> co_await_destroy()
{
	g.add();

	auto id = co_start << []() -> awaitable<void>
	{
		defer(g.done());
		co_await sleep_for(5);
		std::runtime_error("error called !!!\n");
	}();

	LOG_DBG("co_await_destroy sleep begin");
	co_await sleep_for(1);
	LOG_DBG("co_await_destroy destroy");

	co_start.destroy(id);

	LOG_DBG("co_await_destroy sleep end");
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);

	runnable::instance()
		.start(
			[]() -> awaitable<void>
			{
				co_await wait_for_all(co_awaitable(), co_resume());
				LOG_DBG("co_awaitable co_resume finish");
			})
		.start(
			[]() -> awaitable<void>
			{
				co_await wait_for_all(co_awaitable_sleep(), co_awaitable_resume());
				LOG_DBG("co_awaitable_sleep co_awaitable_resume finish");
			})
		.start(co_await_destroy)
		.start(
			[]() -> awaitable<void>
			{
				co_await g.wait();
				LOG_DBG("main finish");
			})
		.wait();
	return 0;
}