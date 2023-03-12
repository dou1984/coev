#include "Chain.h"

namespace coev
{
	Chain *Chain::erase(Chain *_old)
	{
		__list_del(_old->m_prev, _old->m_next);
		_old->__list_clear();
		return _old;
	}
	Chain *Chain::append(Chain *_new)
	{
		if (!_new->empty())
		{
			__list_append(m_prev, this, _new->m_prev, _new->m_next);
			_new->__list_clear();
		}
		return this;
	}
	bool Chain::moveto(Chain *_new)
	{
		if (empty())
			return false;
		if (!_new->empty())
			return false;
		_new->__list_move(m_prev, m_next);
		__list_clear();
		return true;
	}
	void Chain::__list_add(Chain *cur, Chain *prev, Chain *next)
	{
		prev->m_next = next->m_prev = cur;
		cur->m_next = next;
		cur->m_prev = prev;
	}
	void Chain::__list_del(Chain *prev, Chain *next)
	{
		next->m_prev = prev;
		prev->m_next = next;
	}
	void Chain::__list_append(Chain *p, Chain *n, Chain *prev, Chain *next)
	{
		p->m_next = next;
		n->m_prev = prev;
		next->m_prev = p;
		prev->m_next = n;
	}
	void Chain::__list_move(Chain *prev, Chain *next)
	{
		assert(empty());
		prev->m_next = next->m_prev = this;
		m_next = next;
		m_prev = prev;
	}
	void Chain::__list_clear()
	{
		m_next = m_prev = this;
	}
}