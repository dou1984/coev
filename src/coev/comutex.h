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

namespace coev::thread_safe
{
	class comutex final
	{
		async m_listener;
		std::atomic_int m_flag{0};

	public:
		awaitable<void> lock();
		bool unlock();
	};

}