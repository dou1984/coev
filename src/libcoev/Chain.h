/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <stddef.h>
#include <algorithm>
#include <cassert>

namespace coev
{
	class Chain
	{
		Chain *m_prev = this;
		Chain *m_next = this;

	public:
		Chain() = default;
		Chain(Chain &&) = delete;
		Chain(const Chain &) = delete;
		void push_front(Chain *_new) { __list_add(_new, this, m_next); }
		void push_back(Chain *_new) { __list_add(_new, m_prev, this); }
		bool empty() const { return m_prev == this && m_next == this; }
		Chain *pop_front() { return !empty() ? erase(front()) : nullptr; }
		Chain *pop_back() { return !empty() ? erase(back()) : nullptr; }
		Chain *front() { return m_next; }
		Chain *back() { return m_prev; }
		Chain *begin() { return m_next; }
		Chain *end() { return this; }
		Chain *erase(Chain *_old);
		Chain *append(Chain *_new);
		bool moveto(Chain *_new);

	private:
		void __list_add(Chain *cur, Chain *prev, Chain *next);
		void __list_del(Chain *prev, Chain *next);
		void __list_append(Chain *p, Chain *n, Chain *prev, Chain *next);
		void __list_move(Chain *prev, Chain *next);
		void __list_clear();
	};
}
