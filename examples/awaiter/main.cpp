/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coloop.h>

using namespace coev;

std::mutex g_mtx;

struct TestMutex : EVMutex
{
} g_test;

awaiter co_awaiter()
{
	LOG_DBG("awaiter begin\n");
	co_await wait_for<EVMutex>(g_test);
	LOG_DBG("awaiter end\n");
	co_return 0;
}
awaiter co_resume()
{
	LOG_DBG("resume begin\n");
	co_await sleep_for(5);
	LOG_DBG("resume end\n");
	EVMutex *e = &g_test;
	resume(*e, g_mtx);
	co_return 0;
}

int main()
{
	set_log_level(LOG_LEVEL_DEBUG);

	routine::instance()
		.add([]() -> awaiter
			 { co_await wait_for_all(co_awaiter(), co_resume()); })
		
		.join();
	return 0;
}