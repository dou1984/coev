/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev/coev.h>
#include <coev/coev.h>

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

awaitable<int> co_task_t()
{
	proc_task(true);
	co_return 0;
}
awaitable<int> co_task_f()
{
	proc_task(false);
	co_return 0;
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
	throw std::runtime_error("error incompleted");
	co_return 0;
}
awaitable<void> co_two_task()
{
	co_await []() -> awaitable<int>
	{
		LOG_DBG("arrived co_two_task\n");
		co_await sleep_for(1);
		co_await wait_for_any(co_completed(2), wait_for_all(co_incompleted(3), co_incompleted(3)));
		LOG_DBG("arrived co_two_task success\n");
		co_return 0;
	}();
}
awaitable<void> co_two_task2()
{
	co_await []() -> awaitable<int>
	{
		LOG_DBG("arrived co_two_task2\n");
		co_await wait_for_any(co_incompleted(2), wait_for_all(co_completed(1), co_completed(1)));
		LOG_DBG("arrived co_two_task2 success\n");
		co_return 0;
	}();
}
awaitable<void> co_three_task3()
{
	co_await []() -> awaitable<int>
	{
		LOG_DBG("arrived co_three_task3\n");
		co_await sleep_for(1);
		co_await wait_for_any(co_incompleted(2), co_incompleted(2), co_completed(1));
		LOG_DBG("arrived co_three_task3 success\n");
	}();
}
awaitable<void> co_three_task4()
{
	co_await []() -> awaitable<int>
	{
		LOG_DBG("arrived co_three_task3\n");
		co_await sleep_for(1);
		co_await wait_for_all(co_completed(2), co_completed(2), co_completed(1));
		LOG_DBG("arrived co_three_task4 success\n");
	}();
}

auto g_unreachable = []() -> awaitable<int>
{
	defer _([]()
			{ LOG_DBG("arrived defer 1\n"); });
	co_await []() -> awaitable<int>
	{
		defer _([]()
				{ LOG_DBG("arrived defer 2\n"); });
		co_await sleep_for(5);
		throw std::runtime_error("test exception");
		co_return 0;
	}();

	co_return 0;
};
auto g_reachable = []() -> awaitable<int>
{
	co_await sleep_for(1);
	LOG_DBG("arrived co_task5_task3\n");
	co_return 0;
};
awaitable<void> co_task5()
{
	co_await []() -> awaitable<int>
	{
		auto f1 = g_unreachable();
		auto f2 = g_unreachable();
		auto f3 = g_reachable();
		co_task _task;
		_task << f1;
		_task << f2;
		_task << f3;
		co_await _task.wait();
		LOG_DBG("arrived co_task5_end\n");
		co_return 0;
	}();
}

awaitable<void> co_task6()
{
	co_await []() -> awaitable<int>
	{
		co_task _task;
		_task << g_unreachable();
		_task << g_unreachable();
		_task << g_reachable();
		co_await _task.wait();
		LOG_DBG("arrived co_task6_end\n");
		co_return 0;
	}();
}
int main()
{
	set_log_level(LOG_LEVEL_DEBUG);

	runnable::instance()
		// .add(co_task_t)
		// .add(co_task_f)
		// .add(co_two_task)
		// .add(co_two_task2)
		.add(co_three_task3)
		// .add(co_three_task4)
		// .add(co_task5)
		// .add(co_task6)
		.join();
	return 0;
}