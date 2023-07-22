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
	int bind(const char *ip, int port);
	iocontext bind_fd(int);
	iocontext socket();
}