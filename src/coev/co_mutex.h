/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <atomic>
#include <mutex>
#include "awaitable.h"
#include "async.h"

namespace coev
{
	class co_mutex final
	{
	public:
		awaitable<void> lock() noexcept;
		bool unlock() noexcept;
		bool try_lock() noexcept;

	private:
		async m_waiter;
		int m_flag = 0;
	};
	namespace guard
	{
		class co_mutex final
		{
		public:
			awaitable<void> lock() noexcept;
			bool unlock(bool deliver = false) noexcept;
			bool try_lock() noexcept;

		private:
			async m_waiter;
			int m_flag = 0;
		};
	}

}