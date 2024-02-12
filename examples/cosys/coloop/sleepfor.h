/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <memory>
#include <coev.h>

namespace coev
{
	awaiter sleep_for(long);
	awaiter usleep_for(long);

}