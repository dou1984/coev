/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <atomic>
#include "async.h"
#include "awaiter.h"

namespace coev
{
	class waitgroup final : public async<evlts>
	{
		std::atomic_int m_count{0};

	public:
		int add(int c = 1);
		int done();
		awaiter wait();
	};
}