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
#include "async.h"

namespace coev
{
	template <class TYPE>
	class co_channel
	{
	public:
		co_channel()
		{
			m_waiter.push_back(&m_waiter);
		}
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
		awaitable<void> get(TYPE &d)
		{
			if (m_data.empty())
			{
				co_await m_waiter.suspend();
			}
			d = __pop_front();
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
		bool __invalid() const { return m_data.empty(); }
		TYPE __pop_front()
		{
			auto d = std::move(m_data.front());
			m_data.pop();
			return d;
		}
		std::queue<TYPE> m_data;
		async m_waiter;
	};

	namespace guard
	{
		template <class TYPE>
		class co_channel
		{
		public:
			void set(TYPE &&d)
			{
				m_waiter.resume(
					[this, d = std::move(d)]()
					{ m_data.emplace(std::move(d)); });
			}
			void set(const TYPE &d)
			{
				m_waiter.resume(
					[this, &d]()
					{ m_data.push(d); });
			}
			awaitable<void> get(TYPE &d)
			{
				co_await m_waiter.suspend(
					[this]()
					{ return __invalid(); },
					[this, &d]()
					{ d = __pop_front(); });
			}
			bool try_get(TYPE &d)
			{
				std::lock_guard<std::mutex> _(m_waiter.lock());
				if (!__invalid())
				{
					d = __pop_front();
					return true;
				}
				return false;
			}
			auto size() const { return m_data.size(); }

		private:
			std::queue<TYPE> m_data;
			async m_waiter;
			bool __invalid() const { return m_data.empty(); }
			TYPE __pop_front()
			{
				auto d = std::move(m_data.front());
				m_data.pop();
				return d;
			}
		};
	}
}