/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
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
		void push_front(queue *_new) { __list_add(_new, this, m_next); }
		void push_back(queue *_new) { __list_add(_new, m_prev, this); }
		bool empty() const { return m_prev == this && m_next == this; }
		queue *pop_front() { return !empty() ? erase(front()) : nullptr; }
		queue *pop_back() { return !empty() ? erase(back()) : nullptr; }
		queue *front() { return m_next; }
		queue *back() { return m_prev; }
		queue *begin() { return m_next; }
		queue *end() { return this; }
		queue *erase(queue *_old);
		bool moveto(queue *_new);

	private:
		void __list_add(queue *_new, queue *prev, queue *next);
		void __list_del(queue *prev, queue *next);
		void __list_move(queue *prev, queue *next);
		void __list_clear();

		queue *m_prev = this;
		queue *m_next = this;
	};
}
