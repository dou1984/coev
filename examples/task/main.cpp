/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev.h>

using namespace coev;

Awaiter<int> co_task(bool for_all)
{
	auto t0 = []() -> Task
	{
		TRACE();
		co_await sleep_for(2);
		TRACE();
		co_return 0;
	}();

	auto t1 = []() -> Task
	{
		TRACE();
		co_await sleep_for(5);
		TRACE();
		co_return 0;
	}();

	if (for_all)
		co_await wait_for_all(t0, t1);
	else
		co_await wait_for_any(t0, t1);
	TRACE();
	co_return 0;
}

int main()
{
	co_task(false);
	coev::Loop::start();
	return 0;
}