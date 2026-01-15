/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */

#include <coev/coev.h>

using namespace coev;

struct Trigger
{
	int x = 0;
	async m_waiter;
} g_trigger;

void real_call(int __x)
{
	g_trigger.x = __x;
};

// void __async_call(void (*__f)(int))
void __async_call(const std::function<void(int)> &__f)
{
	co_start << [__f]() -> awaitable<int>
	{
		co_await sleep_for(1);
		// __f(1);  // It will core dump here, __f is not equal input argument __f, is it a g++ bug?
		real_call(1); // It's OK.
		g_trigger.m_waiter.resume();
		co_return 0;
	}();
}
awaitable<int> __call()
{
	__async_call(real_call);
	co_await g_trigger.m_waiter.suspend();
	co_return 0;
}

int main()
{
	set_log_level(LOG_LEVEL_DEBUG);
	runnable::instance()
		.start(
			[]() -> awaitable<void>
			{
				LOG_DBG("__call %d", g_trigger.x);
				co_await __call();
				LOG_DBG("__call %d", g_trigger.x);
			})
		.wait();
	return 0;
}