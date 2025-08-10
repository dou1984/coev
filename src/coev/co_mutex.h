/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
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
		async m_waiter;
		int m_flag;

	public:
		awaitable<void> lock();
		bool unlock();
	};

}