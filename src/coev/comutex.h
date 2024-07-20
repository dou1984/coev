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
#include "awaiter.h"
#include "async.h"

namespace coev
{
	class comutex final
	{
		async_ts m_trigger;
		std::atomic_int m_flag{0};

	public:
		awaiter<int> lock();
		bool unlock();
	};
}