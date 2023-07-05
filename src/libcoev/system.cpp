/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <ev.h>
#include "Socket.h"
#include "loop.h"
#include "System.h"
#include "timer.h"

namespace coev
{

	
	awaiter<int> sleep_for(long t)
	{
		timer _timer(t, 0);
		_timer.active();
		co_await wait_for<EVTimer>(_timer);
		co_return 0;
	}
	awaiter<int> usleep_for(long t)
	{
		timer _timer((float)t / 1000000, 0);
		_timer.active();
		co_await wait_for<EVTimer>(_timer);
		co_return 0;
	}
}