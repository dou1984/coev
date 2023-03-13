/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include "Loop.h"
#include "IOContext.h"

namespace coev
{
	struct Udp final
	{
		static IOContext bind(const char *ip, int port);
		static IOContext socket();
	};
}