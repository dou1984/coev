/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <mutex>
#include "chain.h"
#include "object.h"
#include "awaiter.h"
#include "event.h"

namespace coev
{
	struct async : chain, OBJECT0
	{
	};
	struct async_ext : chain, OBJECT1
	{
	};
	struct async_mutex : chain, std::mutex
	{
	};

}