/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <atomic>
#include "trigger.h"
#include "awaiter.h"

namespace coev
{
	class waitgroup final
	{
		ts::trigger m_trigger;
		std::atomic_int m_count{0};

	public:
		int add(int c = 1);
		int done();
		awaiter wait();
	};
}