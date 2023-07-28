/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coloop.h>

using namespace coev;

void __func(const std::function<void(int, int)> &__f)
{
	[&]() -> awaiter
	{
		TRACE();
		co_await sleep_for(5);
		TRACE();
		__f(1, 2);
		TRACE();
		co_return 0;
	}();
}
awaiter __call(int *x, int *y)
{
	EVEvent ev;
	auto f = [&](int __x, int __y)
	{
		*x = __x;
		*y = __y;
		TRACE();
		ev.resume();
		TRACE();
	};
	__func(f);
	TRACE();
	co_await wait_for<EVEvent>(ev);
	TRACE();
	co_return 0;
}

int main()
{
	set_log_level(LOG_LEVEL_CORE);
	routine::instance()
		.add([]() -> awaiter
			 { 
				int x = 0;
				int y = 0;
				TRACE();
				co_await __call(&x, &y);
				LOG_DBG("__call %d:%d\n", x, y);
				co_return 0; })
		.join();
	return 0;
}