/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "queue.h"

namespace coev
{
	queue *queue::erase(queue *_old)
	{
		__list_del(_old->m_prev, _old->m_next);
		_old->__list_clear();
		return _old;
	}
	bool queue::move_to(queue *_new)
	{
		if (empty())
			return false;
		if (!_new->empty())
			return false;
		_new->__list_move(m_prev, m_next);
		__list_clear();
		return true;
	}
	void queue::__list_add(queue *_new, queue *prev, queue *next)
	{
		prev->m_next = next->m_prev = _new;
		_new->m_next = next;
		_new->m_prev = prev;
	}
	void queue::__list_del(queue *prev, queue *next)
	{
		next->m_prev = prev;
		prev->m_next = next;
	}
	void queue::__list_move(queue *prev, queue *next)
	{
		assert(empty());
		prev->m_next = next->m_prev = this;
		m_next = next;
		m_prev = prev;
	}
	void queue::__list_clear()
	{
		m_next = m_prev = this;
	}
}