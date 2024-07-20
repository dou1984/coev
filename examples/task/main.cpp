/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "../cosys/coloop.h"

using namespace coev;

awaiter<int> co_task(bool for_all)
{
	auto t0 = []() -> awaiter<int>
	{
		TRACE();
		co_await sleep_for(2);
		TRACE();
		co_return 0;
	}();

	auto t1 = []() -> awaiter<int>
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

awaiter<int> co_task_short(bool for_all)
{
	if (for_all)
		co_await wait_for_all(sleep_for(2), sleep_for(3));
	else
		co_await wait_for_any(sleep_for(2), sleep_for(3));
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
void co_task_u()
{
	co_task_short(true);
}
void co_task_v()
{
	co_task_short(false);
}
awaiter<int> co_completed()
{
	LOG_DBG("incompleted begin\n")
	co_await sleep_for(2);
	LOG_DBG("arrived success\n");
	co_return 0;
}
awaiter<int> co_incompleted()
{
	LOG_DBG("incompleted begin\n")
	co_await sleep_for(3);
	LOG_DBG("arrived error\n");
	co_return 0;
}
void co_two_task()
{
	[]() -> awaiter<int>
	{
		co_await wait_for_any(co_completed(), wait_for_all(co_incompleted(), co_incompleted()));
		co_return 0;
	}();
}
void co_two_task2()
{
	[]() -> awaiter<int>
	{
		co_await wait_for_any(co_incompleted(), wait_for_all(co_completed(), co_completed()));
		co_return 0;
	}();
}

int main()
{
	set_log_level(LOG_LEVEL_DEBUG);

	running::instance()
		.add(co_task_t)
		.add(co_task_f)
		.add(co_task_u)
		.add(co_task_v)
		.add(co_two_task)
		.add(co_two_task2)		
		.join();
	return 0;
}