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
#include "trigger.h"

namespace coev
{
	class comutex final
	{
		ts::trigger m_trigger;
		std::atomic_int m_flag{0};

	public:
		awaiter lock();
		awaiter unlock();
	};
}