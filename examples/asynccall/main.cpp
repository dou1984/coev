/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "../cosys/coloop.h"

using namespace coev;

struct Trigger
{
	int x = 0;
	async m_trigger;
} g_trigger;

void real_call(int __x)
{
	g_trigger.x = __x;
};

void __async_call(void (*__f)(int))
{
	[__f]() -> awaiter<int>
	{
		co_await sleep_for(1);
		//__f(1); // It will core dump here, __f is not equal input argument __f, is it a g++ bug?
		real_call(1); // It's OK.
		coev::resume(g_trigger.m_trigger);
		co_return 0;
	}();
}
awaiter<int> __call()
{
	__async_call(real_call);
	co_await wait_for(g_trigger.m_trigger);
	co_return 0;
}

int main()
{
	set_log_level(LOG_LEVEL_CORE);
	running::instance()
		.add([]() -> awaiter<void>
			 { 			
				co_await __call();
				LOG_DBG("__call %d\n", g_trigger.x); })
		.join();
	return 0;
}