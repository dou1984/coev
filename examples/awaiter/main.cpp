/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coloop.h>

using namespace coev;

std::recursive_mutex g_mtx;

struct TestMutex : EVRecv
{
} g_test;

awaiter co_awaiter()
{
	LOG_DBG("awaiter begin\n");
	co_await wait_for<EVRecv>(&g_test);
	LOG_DBG("awaiter end\n");
	co_return 0;
}
awaiter co_resume()
{
	LOG_DBG("resume begin\n");
	co_await sleep_for(5);
	LOG_DBG("resume end\n");
	EVRecv *e = &g_test;
	resume<EVRecv>(e);
	co_return 0;
}

awaiter co_awaiter_sleep()
{

	auto ev = wait_for<EVRecv>(&g_test);

	LOG_DBG("awaiter sleep\n");
	co_await sleep_for(2);
	LOG_DBG("awaiter begin\n");
	co_await ev;
	LOG_DBG("awaiter end\n");
	co_return 0;
}
awaiter co_awaiter_resume()
{
	co_await sleep_for(1);
	LOG_DBG("awaiter resume\n");
	resume<EVRecv>(&g_test);
	co_return 0;
}
int main()
{
	set_log_level(LOG_LEVEL_DEBUG);

	running::instance()
		/*
			.add([]() -> awaiter
				 { co_return co_await wait_for_all(co_awaiter(), co_resume()); })
				 */
		.add([]() -> awaiter
			 { co_return co_await wait_for_all(co_awaiter_sleep(), co_awaiter_resume()); })
		.join();
	return 0;
}