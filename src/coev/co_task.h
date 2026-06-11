/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <memory>
#include <functional>
#include <map>
#include <set>
#include "queue.h"
#include "co_async.h"
#include "co_event.h"
#include "is_destroying.h"

namespace coev
{
#define co_start coev::local<coev::guard::co_task>::instance()

	class co_task final
	{
		co_async m_waiter;
		std::map<promise *, int64_t> m_promises;
		std::set<int64_t> m_ids;
		int64_t m_id = 0;

	public:
		virtual ~co_task();
		void destroy();
		void destroy(int64_t id);

		template <typename T>
		int operator<<(T &&t)
		{
			auto p = t.move();
			return load(p);
		}
		int load(promise *);
		int unload(promise *);
		bool empty();
		awaitable<void> wait_all();
		awaitable<int64_t> wait();
	};

	namespace guard
	{
		class co_task final
		{
			guard::co_async m_waiter;
			std::map<promise *, int64_t> m_promises;
			std::set<int64_t> m_ids;
			int64_t m_id = 0;

		public:
			virtual ~co_task();
			void destroy();
			void destroy(int64_t id);

			template <typename T>
			int operator<<(T &&t)
			{
				auto p = t.move();
				return load(p);
			}
			int load(promise *);
			int unload(promise *);
			bool empty();
			awaitable<void> wait_all();
			awaitable<int64_t> wait();
		};
	}
}
