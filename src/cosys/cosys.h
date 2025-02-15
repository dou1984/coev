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
#include "client.h"
#include "server.h"
#include "serverpool.h"
#include "sleepfor.h"
#include "running.h"
#include "udp.h"
#include "co_timer.h"

#define max_ev_loop (0x100)

namespace coev
{
	struct cosys
	{
		static void start();
		static struct ev_loop *data();
		static struct ev_loop *at(uint64_t);
		static void stop();
	};
}
