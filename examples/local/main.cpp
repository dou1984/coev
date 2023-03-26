/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev.h>

using namespace coev;

awaiter<int> test_sleep(int x)
{
	auto c = Local::ref();
	LOG_DBG("sleep begin %d\n", x);
	co_await sleep_for(x);
	LOG_DBG("sleep end %d\n", x);
	co_return 0;
}
int test_calc()
{
	test_sleep(1);
	test_sleep(2);
	return 0;
}
awaiter<int> test()
{
	test_calc();
	LOG_DBG("wait_for_local begin\n");
	co_await wait_for_local();
	LOG_DBG("wait_for_local end\n");
	co_return 0;
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);
	Routine r;
	r.add(test);
	r.join();
	return 0;
}