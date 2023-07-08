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
#include "eventchain.h"

namespace coev
{
	class cowaiter final : EVMutex
	{
	public:
		int add();
		int done();
		awaiter<int> wait();

	private:
		std::mutex m_lock;
		int m_count = 0;
	};
}