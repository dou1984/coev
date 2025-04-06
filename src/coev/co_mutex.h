/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <atomic>
#include <mutex>
#include "awaitable.h"
#include "async.h"

namespace coev::guard
{
	class co_mutex final
	{
		std::atomic_int m_flag{0};

	public:
		async m_waiter;
		awaitable<void> lock();
		bool unlock();
	};

}