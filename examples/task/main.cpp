/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <coev/coev.h>

using namespace coev;

awaitable<int> proc_task(bool for_all)
{
	auto t0 = []() -> awaitable<int>
	{
		LOG_DBG("arrived t0 sleep");
		co_await sleep_for(1);
		LOG_DBG("arrived t0 success");
		co_return 0;
	}();

	auto t1 = []() -> awaitable<int>
	{
		LOG_DBG("arrived t1 sleep");
		co_await sleep_for(1);
		LOG_DBG("arrived t1 success")
		co_return 0;
	}();

	if (for_all)
		co_await wait_for_all(t0, t1);
	else
		co_await wait_for_any(t0, t1);
	LOG_DBG("arrived proc_task success")
	co_return 0;
}

awaitable<void> co_task_t()
{
	co_await proc_task(true);
}
awaitable<void> co_task_f()
{
	co_await proc_task(false);
}

awaitable<int> co_completed(int t)
{
	LOG_DBG("arrived co_completed");
	co_await sleep_for(t);
	LOG_DBG("arrived success co_completed");
	co_return 0;
}
awaitable<int> co_incompleted(int t)
{
	LOG_DBG("arrived co_incompleted");
	co_await sleep_for(t);
	LOG_DBG("arrived error co_incompleted");
	throw std::runtime_error("error incompleted");
	co_return 0;
}
awaitable<void> co_two_task()
{
	co_await []() -> awaitable<int>
	{
		LOG_DBG("arrived co_two_task");
		co_await sleep_for(1);
		co_await wait_for_any(co_completed(2), wait_for_all(co_incompleted(3), co_incompleted(3)));
		LOG_DBG("arrived co_two_task success");
		co_return 0;
	}();
}
awaitable<void> co_two_task2()
{
	co_await []() -> awaitable<int>
	{
		LOG_DBG("arrived co_two_task2");
		co_await wait_for_any(co_incompleted(2), wait_for_all(co_completed(1), co_completed(1)));
		LOG_DBG("arrived co_two_task2 success");
		co_return 0;
	}();
}
awaitable<void> co_three_task3()
{
	co_await []() -> awaitable<int>
	{
		LOG_DBG("arrived co_three_task3");
		co_await sleep_for(1);
		co_await wait_for_any(co_incompleted(2), co_incompleted(2), co_completed(1));
		LOG_DBG("arrived co_three_task3 success");
	}();
}
awaitable<void> co_three_task4()
{
	co_await []() -> awaitable<int>
	{
		LOG_DBG("arrived co_three_task3");
		co_await sleep_for(1);
		co_await wait_for_all(co_completed(2), co_completed(2), co_completed(1));
		LOG_DBG("arrived co_three_task4 success");
	}();
}

auto g_unreachable = []() -> awaitable<int>
{
	co_await []() -> awaitable<int>
	{
		co_await sleep_for(5);
		LOG_DBG("arrived error 2");
		throw std::runtime_error("test exception");
		co_return 0;
	}();
	LOG_DBG("arrived error 1");
	co_return 0;
};
auto g_reachable = []() -> awaitable<int>
{
	co_await sleep_for(1);
	LOG_DBG("arrived co_task5_task3");
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
		LOG_DBG("arrived co_task5_end");
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
		LOG_DBG("arrived co_task6_end");
		co_return 0;
	}();
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);

	runnable::instance()
		.start(co_task_t)
		.start(co_task_f)
		.start(co_two_task)
		.start(co_two_task2)
		.start(co_three_task3)
		.start(co_three_task4)
		.start(co_task5)
		.start(co_task6)
		.wait();
	return 0;
}