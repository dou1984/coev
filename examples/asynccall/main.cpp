/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coloop.h>

using namespace coev;

struct Trigger : EVEvent
{
	int x = 0;
} g_trigger;

void real_call(int __x){
	// g_trigger.x = __x;
};

void __async_call(void (*__f)(int))
{
	[=]() -> awaiter
	{
		TRACE();
		co_await sleep_for(1);
		__f(1); // It will core dump here, __f is not equal input argument __f, is it a g++ bug?
		// real_call(1); // It's OK.
		TRACE();
		g_trigger.EVEvent::resume();
		TRACE();
		co_return 0;
	}();
}
awaiter __call()
{
	TRACE();
	__async_call(real_call);
	TRACE();
	co_await g_trigger.EVEvent::wait_for();
	TRACE();
	co_return 0;
}

int main()
{
	set_log_level(LOG_LEVEL_CORE);
	running::instance()
		.add([]() -> awaiter
			 { 			
				TRACE();
				co_await __call();
				LOG_DBG("__call %d\n", g_trigger.x);
				co_return 0; })
		.join();
	return 0;
}