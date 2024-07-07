/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <mutex>
#include <tuple>
#include "chain.h"

namespace coev
{
	struct async : chain
	{
	};

	namespace ts
	{
		struct async : chain, std::mutex
		{
		};
	}
}