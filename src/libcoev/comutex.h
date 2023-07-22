/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <atomic>
#include "awaiter.h"
#include "eventchain.h"

namespace coev
{
	class comutex final : EVMutex
	{
	public:
		awaiter lock();
		awaiter unlock();

	private:
		const int on = 1;
		const int off = 0;
		std::mutex m_lock;
		int m_flag = 0;
	};
}