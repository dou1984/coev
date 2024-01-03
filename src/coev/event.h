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
		std::coroutine_handle<> m_awaiting = nullptr;
		uint64_t m_tid = 0;
		std::atomic_bool m_ready{false};
		template <class EVENTCHAIN>
		event(EVENTCHAIN *_eventchain) : m_tid(gtid())
		{
			if (_eventchain != nullptr)
				_eventchain->append(this);
		}
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
