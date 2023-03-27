/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include "loop.h"
#include "iocontext.h"

namespace coev::udp
{
	iocontext bind(const char *ip, int port);
	iocontext socket();
}