/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "promise.h"
#include "log.h"
#include "awaiter.h"

namespace coev::details
{
	promise::~promise()
	{
		LOG_CORE("promise_type:%p awaiter:%p\n", this, m_awaiter);
		if (m_awaiter)
		{
			m_awaiter->m_state = STATUS_READY;
			m_awaiter->resume();
			m_awaiter = nullptr;
		}
	}
	void promise::unhandled_exception()
	{
		throw std::current_exception();
	}
	std::suspend_never promise::initial_suspend()
	{
		return {};
	}
	std::suspend_never promise::final_suspend() noexcept
	{
		return {};
	}
}