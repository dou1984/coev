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
#include "awaitable.h"

namespace coev
{
	class waitgroup final
	{
		ts::async m_listener;
		std::atomic_int m_count{0};

	public:
		int add(int c = 1);
		void done();
		awaitable<void> wait();
	};
}