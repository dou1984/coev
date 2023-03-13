/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <stdint.h>
#include <ev.h>
#include "Log.h"
#include "Event.h"
#define max_ev_loop (0x1000)

#define WHILE(...) while(__VA_ARGS__)

namespace coev
{
	struct Loop final
	{
		static void start();
		static struct ev_loop *data();
		static struct ev_loop *at(uint32_t);
		static uint32_t tag();
		static void resume(Event *);
	};
}
