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
		std::coroutine_handle<> m_awaiting = nullptr;
		uint64_t m_tid = 0;
		event(chain *eventchain, uint64_t _tid);
		event(chain *eventchain);
		virtual ~event();
		event(event &&) = delete;
		event(const event &) = delete;
		void await_resume();
		bool await_ready();
		void await_suspend(std::coroutine_handle<> awaiting);
		void resume();
	};
	using RESUME = void (*)(event *ev);

}
