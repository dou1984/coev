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
	class chain
	{
	public:
		chain() = default;
		chain(chain &&) = delete;
		chain(const chain &) = delete;
		virtual ~chain() = default;
		void push_front(chain *_new) { __list_add(_new, this, m_next); }
		void push_back(chain *_new) { __list_add(_new, m_prev, this); }
		bool empty() const { return m_prev == this && m_next == this; }
		chain *pop_front() { return !empty() ? erase(front()) : nullptr; }
		chain *pop_back() { return !empty() ? erase(back()) : nullptr; }
		chain *front() { return m_next; }
		chain *back() { return m_prev; }
		chain *begin() { return m_next; }
		chain *end() { return this; }
		chain *erase(chain *_old);
		bool moveto(chain *_new);

	private:
		void __list_add(chain *_new, chain *prev, chain *next);
		void __list_del(chain *prev, chain *next);
		void __list_move(chain *prev, chain *next);
		void __list_clear();

		chain *m_prev = this;
		chain *m_next = this;
	};
}
