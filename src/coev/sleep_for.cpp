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
#include "defer.h"

namespace coev
{
	awaitable<void> sleep_for(double t)
	{
		co_timer _timer(t, 0);
		LOG_CORE("sleep_for %fs", t);
		_timer.active();
		co_await _timer.suspend();
	}
	awaitable<void> usleep_for(double t)
	{
		co_timer _timer(t / 1000000, 0);
		LOG_CORE("usleep_for %fus", t);
		_timer.active();
		co_await _timer.suspend();
	}
}