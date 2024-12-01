/*
 *	cosys - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include "iocontext.h"

namespace coev::udp
{
	int bindfd(const char *ip, int port);
	int socketfd();
}