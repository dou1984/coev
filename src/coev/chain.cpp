/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "chain.h"

namespace coev
{
	chain *chain::erase(chain *_old)
	{
		__list_del(_old->m_prev, _old->m_next);
		_old->__list_clear();
		return _old;
	}
	/*
	chain *chain::append(chain *_new)
	{
		if (!_new->empty())
		{
			__list_append(m_prev, this, _new->m_prev, _new->m_next);
			_new->__list_clear();
		}
		return this;
	}
	*/
	bool chain::moveto(chain *_new)
	{
		if (empty())
			return false;
		if (!_new->empty())
			return false;
		_new->__list_move(m_prev, m_next);
		__list_clear();
		return true;
	}
	void chain::__list_add(chain *_new, chain *prev, chain *next)
	{
		prev->m_next = next->m_prev = _new;
		_new->m_next = next;
		_new->m_prev = prev;
	}
	void chain::__list_del(chain *prev, chain *next)
	{
		next->m_prev = prev;
		prev->m_next = next;
	}
	/*
	void chain::__list_append(chain *new_prev, chain *new_next, chain *prev, chain *next)
	{
		new_prev->m_next = next;
		new_next->m_prev = prev;
		next->m_prev = p;
		prev->m_next = n;
	}
	*/
	void chain::__list_move(chain *prev, chain *next)
	{
		assert(empty());
		prev->m_next = next->m_prev = this;
		m_next = next;
		m_prev = prev;
	}
	void chain::__list_clear()
	{
		m_next = m_prev = this;
	}
}