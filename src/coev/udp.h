/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include "io_context.h"

namespace coev::udp
{
	int bindfd(const char *ip, int port);
	int socketfd();
}