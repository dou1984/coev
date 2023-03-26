/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <stdint.h>
#include <ev.h>
#include "log.h"
#include "event.h"
#define max_ev_loop (0x1000)

namespace coev
{
	struct loop final
	{
		static void start();
		static struct ev_loop *data();
		static struct ev_loop *at(uint32_t);
		static uint32_t tag();
		static void resume(event *);
	};
}
