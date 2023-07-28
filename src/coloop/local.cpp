/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <memory>
#include "loop.h"
#include "local.h"

namespace coev
{
	thread_local std::shared_ptr<local> g_local = std::make_shared<local>();
	localext::localext(std::shared_ptr<local> &_) : m_current(_)
	{
		TRACE();
		assert(m_current);
		++m_current->m_ref;
	}
	localext::~localext()
	{
		TRACE();
		assert(m_current);
		if (--m_current->m_ref == 0)
			m_current->EVRecv::resume();
	}
	std::unique_ptr<localext> local::ref()
	{
		return std::make_unique<localext>(g_local);
	}
	awaiter wait_for_local()
	{
		auto _this = g_local;
		g_local = std::make_shared<local>();
		co_await wait_for<EVRecv>(*_this);
		co_return 0;
	}
}
