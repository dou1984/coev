/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <coroutine>
#include "log.h"
#include "chain.h"
#include "promise.h"

namespace coev
{
	struct event final : chain
	{
		chain *m_eventchain = nullptr;
		std::coroutine_handle<> m_awaiting = nullptr;
		uint32_t m_tag = 0;
		event(chain *obj, uint32_t _tag);
		event(chain *obj);
		virtual ~event();
		event(event &&) = delete;
		event(const event &) = delete;
		void await_resume();
		bool await_ready();
		void await_suspend(std::coroutine_handle<> awaiting);
		void resume();
	};
}
