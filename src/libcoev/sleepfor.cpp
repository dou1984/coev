/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <ev.h>
#include "loop.h"
#include "sleepfor.h"
#include "timer.h"
#include "waitfor.h"

namespace coev
{
	awaiter sleep_for(long t)
	{
		timer _timer(t, 0);
		_timer.active();
		co_await wait_for<EVTimer>(_timer);
		co_return 0;
	}
	awaiter usleep_for(long t)
	{
		timer _timer((float)t / 1000000, 0);
		_timer.active();
		co_await wait_for<EVTimer>(_timer);
		co_return 0;
	}
}