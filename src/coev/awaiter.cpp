/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "awaiter.h"
#include "promise.h"

namespace coev::details
{
	bool awaiter_impl::await_ready()
	{
		return m_state == STATUS_READY;
	}
	void awaiter_impl::await_suspend(std::coroutine_handle<> caller)
	{
		m_caller = caller;
		m_state = STATUS_SUSPEND;
	}
	void awaiter_impl::resume()
	{
		m_state = STATUS_READY;
		//LOG_CORE("m_caller:%p m_callee:%p\n", m_caller ? m_caller.address() : 0, m_callee ? m_callee.address() : 0);
		if (m_caller.address() && !m_caller.done())
			m_caller.resume();
		taskevent::__resume();
	}
}