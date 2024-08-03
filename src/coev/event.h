/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <coroutine>
#include <atomic>
#include "log.h"
#include "chain.h"
#include "promise.h"
#include "gtid.h"

namespace coev
{
	struct event final : chain
	{
		std::atomic_int m_status{STATUS_INIT};
		std::coroutine_handle<> m_awaiter = nullptr;
		size_t m_tid;
		event(chain *_eventchain);
		virtual ~event();
		event(event &&) = delete;
		event(const event &) = delete;
		void await_resume();
		bool await_ready();
		void await_suspend(std::coroutine_handle<> awaitable);
		void resume();
	};
}
