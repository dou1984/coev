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
	int x;
	int y;
} g_trigger;

void real_call(int __x, int __y)
{
	TRACE();
	g_trigger.x = __x;
	g_trigger.y = __y;
	TRACE();
	g_trigger.EVEvent::resume();
	TRACE();
};

void __async_call()
{
	[&]() -> awaiter
	{
		TRACE();
		co_await sleep_for(1);
		real_call(1, 2);
		TRACE();
		co_return 0;
	}();
}
awaiter __call()
{
	TRACE();
	__async_call();
	TRACE();
	co_await wait_for<EVEvent>(g_trigger);
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
				LOG_DBG("__call %d:%d\n", g_trigger.x, g_trigger.y);
				co_return 0; })
		.join();
	return 0;
}