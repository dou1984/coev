/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <ev.h>
#include "cosys.h"
#include "sleep_for.h"
#include "co_timer.h"

namespace coev
{
	awaitable<void> sleep_for(long t)
	{
		co_timer _timer(t, 0);
		LOG_CORE("sleep_for %lds\n", t);
		_timer.active();
		co_await _timer.suspend();
	}
	awaitable<void> usleep_for(long t)
	{
		co_timer _timer((float)t / 1000000, 0);
		LOG_CORE("usleep_for %ldus\n", t);
		_timer.active();
		co_await _timer.suspend();
	}
}