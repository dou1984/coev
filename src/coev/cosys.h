/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#pragma once
#include <stdint.h>
#include <ev.h>


#define max_ev_loop (0x100)

namespace coev
{
	struct cosys
	{
		static void start();
		static struct ev_loop *data();
		static void stop();
	};
}
