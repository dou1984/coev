/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev.h>

using namespace coev;

awaiter<int> co_task(bool for_all)
{
	auto t0 = []() -> task
	{
		TRACE();
		co_await sleep_for(2);
		TRACE();
		co_return 0;
	}();

	auto t1 = []() -> task
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

void co_task_t()
{
	co_task(true);
}
void co_task_f()
{
	co_task(false);
}

int main()
{
	set_log_level(LOG_LEVEL_CORE);
	Routine r;
	r.add(co_task_t);
	r.add(co_task_f);
	r.join();
	return 0;
}