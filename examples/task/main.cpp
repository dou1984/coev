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
		LOG_DBG("arrived t0 sleep\n");
		co_await sleep_for(1);
		LOG_DBG("arrived t0 success\n");
		co_return 0;
	}();

	auto t1 = []() -> awaitable<int>
	{
		LOG_DBG("arrived t1 sleep\n");
		co_await sleep_for(1);
		LOG_DBG("arrived t1 success\n")
		co_return 0;
	}();

	if (for_all)
		co_await wait_for_all(t0, t1);
	else
		co_await wait_for_any(t0, t1);
	LOG_DBG("arrived proc_task success\n")
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

awaitable<int> co_completed(int t)
{
	LOG_DBG("arrived co_completed\n");
	co_await sleep_for(t);
	LOG_DBG("arrived success co_completed\n");
	co_return 0;
}
awaitable<int> co_incompleted(int t)
{
	LOG_DBG("arrived co_incompleted\n");
	co_await sleep_for(t);
	LOG_DBG("arrived error co_incompleted\n");
	co_return 0;
}
void co_two_task()
{
	[]() -> awaitable<int>
	{
		LOG_DBG("arrived co_two_task\n");
		co_await sleep_for(1);
		co_await wait_for_any(co_completed(2), wait_for_all(co_incompleted(3), co_incompleted(3)));
		LOG_DBG("arrived co_two_task success\n");
		co_return 0;
	}();
}
void co_two_task2()
{
	[]() -> awaitable<int>
	{
		LOG_DBG("arrived co_two_task2\n");
		co_await wait_for_any(co_incompleted(2), wait_for_all(co_completed(1), co_completed(1)));
		LOG_DBG("arrived co_two_task2 success\n");
		co_return 0;
	}();
}

void co_three_task3()
{
	[]() -> awaitable<int>
	{
		LOG_DBG("arrived co_three_task3\n");
		co_await sleep_for(1);
		co_await wait_for_any(co_incompleted(2), co_incompleted(2), co_completed(1));
		LOG_DBG("arrived co_three_task3 success\n");
	}();
}
void co_three_task4()
{
	[]() -> awaitable<int>
	{
		LOG_DBG("arrived co_three_task3\n");
		co_await sleep_for(1);
		co_await wait_for_all(co_completed(2), co_completed(2), co_completed(1));
		LOG_DBG("arrived co_three_task4 success\n");
	}();
}

void co_task5()
{
	[]() -> awaitable<int>
	{
		co_task _task;
		_task.insert(
			[]() -> awaitable<int>
			{
				defer _([]()
						{ LOG_DBG("arrived defer 1\n"); });
				co_await []() -> awaitable<int>
				{
					defer _([]()
							{ LOG_DBG("arrived defer 2\n"); });
					co_await sleep_for(10);
					co_return 0;
				}();

				co_return 0;
			}());
		_task.insert(
			[]() -> awaitable<int>
			{
				co_await sleep_for(10);
				co_return 0;
			}());
		_task.insert(
			[]() -> awaitable<int>
			{
				co_await sleep_for(1);
				co_return 0;
			}());
		co_await _task.wait();
		co_return 0;
	}();
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);

	running::instance()
		// .add(co_task_t)
		// .add(co_task_f)
		// .add(co_two_task)
		// .add(co_two_task2)
		// .add(co_three_task3)
		// .add(co_three_task4)
		.add(co_task5)
		.join();
	return 0;
}