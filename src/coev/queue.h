/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <stddef.h>
#include <algorithm>
#include <cassert>

namespace coev
{
	class queue
	{
	public:
		queue() = default;
		queue(queue &&) = delete;
		queue(const queue &) = delete;
		virtual ~queue() = default;
		const queue &operator=(const queue &) = delete;
		const queue &operator=(queue &&) = delete;
		void push_front(queue *_new) noexcept { __list_add(_new, this, m_next); }
		void push_back(queue *_new) noexcept { __list_add(_new, m_prev, this); }
		bool empty() const noexcept { return m_prev == this && m_next == this; }
		queue *pop_front() noexcept { return !empty() ? erase(front()) : nullptr; }
		queue *pop_back() noexcept { return !empty() ? erase(back()) : nullptr; }
		queue *front() noexcept { return m_next; }
		queue *back() noexcept { return m_prev; }
		queue *begin() noexcept { return m_next; }
		queue *end() noexcept { return this; }
		queue *erase(queue *_old) noexcept;
		bool move_to(queue *_new) noexcept;

	private:
		void __list_add(queue *_new, queue *prev, queue *next) noexcept;
		void __list_del(queue *prev, queue *next) noexcept;
		void __list_move(queue *prev, queue *next) noexcept;
		void __list_clear() noexcept;

		queue *m_prev = this;
		queue *m_next = this;
	};
}
