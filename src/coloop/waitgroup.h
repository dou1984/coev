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
#include "../coev.h"

namespace coev
{
	class waitgroup final : EVMutex
	{
	public:
		int add(int c = 1);
		int done();
		awaiter wait();

	private:
		std::mutex m_lock;
		int m_count = 0;
	};
}