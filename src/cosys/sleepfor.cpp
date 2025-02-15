/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <ev.h>
#include "cosys.h"
#include "sleepfor.h"
#include "co_timer.h"

namespace coev
{
	awaitable<void> sleep_for(long t)
	{
		co_timer _timer(t, 0);
		_timer.active();
		co_await _timer.suspend();
	}
	awaitable<void> usleep_for(long t)
	{
		co_timer _timer((float)t / 1000000, 0);
		_timer.active();
		co_await _timer.suspend();
	}
}