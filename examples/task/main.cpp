/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev/coev.h>
#include <cosys/cosys.h>

using namespace coev;

awaitable<int> proc_task(bool for_all)
{
	auto t0 = []() -> awaitable<int>
	{
		TRACE();
		co_return 0;
	}();

	auto t1 = []() -> awaitable<int>
	{
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
	proc_task(true);
}
void co_task_f()
{
	proc_task(false);
}

awaitable<int> co_completed()
{

	LOG_DBG("arrived success\n");
	co_return 0;
}
awaitable<int> co_incompleted()
{

	LOG_DBG("arrived error\n");
	co_return 0;
}
void co_two_task()
{
	[]() -> awaitable<int>
	{
		co_await wait_for_any(co_completed(), wait_for_all(co_incompleted(), co_incompleted()));
		co_return 0;
	}();
}
void co_two_task2()
{
	[]() -> awaitable<int>
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
		.add(co_two_task)
		.add(co_two_task2)		
		.join();
	return 0;
}