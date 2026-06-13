/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <queue>
#include "awaitable.h"
#include "wait_for.h"
#include "co_async.h"
#include "co_deliver.h"

namespace coev
{
	template <class TYPE>
	class co_channel : queue
	{
	public:
		co_channel() = default;
		void set(TYPE &&d)
		{
			m_data.emplace(std::move(d));
			m_waiter.resume_next_loop();
		}
		void set(const TYPE &d)
		{
			m_data.push(d);
			m_waiter.resume_next_loop();
		}
		awaitable<int64_t> get(TYPE &d)
		{
			if (m_data.empty())
			{
				auto r = co_await m_waiter.suspend();
				if (r == INVALID)
				{
					co_return r;
				}
			}
			d = __pop_front();
			co_return 0;
		}
		bool try_get(TYPE &d)
		{
			if (m_data.empty())
			{
				return false;
			}
			d = __pop_front();
			return true;
		}
		auto size() const { return m_data.size(); }

	private:
		bool __empty() const { return m_data.empty(); }
		TYPE __pop_front()
		{
			auto d = std::move(m_data.front());
			m_data.pop();
			return d;
		}
		std::queue<TYPE> m_data;
		co_async m_waiter;
	};

	namespace guard
	{
		template <class TYPE>
		class co_channel
		{
		public:
			void set(TYPE &&d)
			{
				m_waiter.deliver(
					[this, d = std::move(d)]()
					{ m_data.emplace(std::move(d)); });
			}
			void set(const TYPE &d)
			{
				m_waiter.deliver(
					[this, &d]()
					{ m_data.push(d); });
			}
			awaitable<int64_t> get(TYPE &d)
			{
				co_return co_await m_waiter.suspend(
					[this]()
					{ return __empty(); },
					[this, &d]()
					{ d = __pop_front(); });
			}
			bool try_get(TYPE &d)
			{
				std::lock_guard<std::mutex> _(m_waiter.lock());
				if (!__empty())
				{
					d = __pop_front();
					return true;
				}
				return false;
			}
			auto size() const { return m_data.size(); }

		private:
			std::queue<TYPE> m_data;
			co_async m_waiter;
			bool __empty() const { return m_data.empty(); }
			TYPE __pop_front()
			{
				auto d = std::move(m_data.front());
				m_data.pop();
				return d;
			}
		};
	}
}