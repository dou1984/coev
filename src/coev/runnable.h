/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <thread>
#include <list>
#include <functional>
#include "singleton.h"
#include "cosys.h"
#include "awaitable.h"

namespace coev
{
	class runnable final : public singleton<runnable>
	{
		using func = std::function<awaitable<void>()>;
		std::list<std::thread> m_list;

		void __add(const func &_f);

	public:
		runnable();
		runnable(const runnable &) = delete;
		runnable(runnable &&) = delete;
		runnable &operator<<(const func &_f);
		runnable &add(int count, const func &_f);

		void join();
		void detach();
	};
}