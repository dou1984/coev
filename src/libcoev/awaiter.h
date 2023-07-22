/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <coroutine>
#include "chain.h"
#include "Promise.h"
#include "log.h"
#include "object.h"
#include "hook.h"
#include "taskext.h"

namespace coev
{
	class awaiter final : public taskext
	{
	public:
		struct promise_type : promise
		{
			int value;
			awaiter *_this = nullptr;
			promise_type() = default;
			~promise_type();
			awaiter get_return_object();
			std::suspend_never return_value(int v);
			std::suspend_never yield_value(int v);
		};
		awaiter() = default;
		awaiter(std::coroutine_handle<promise_type> h);

		awaiter(awaiter &&o) = delete;
		awaiter(const awaiter &) = delete;
		const awaiter &operator=(awaiter &&) = delete;
		~awaiter();
		void resume();
		bool done() { return m_coroutine ? m_coroutine.done() : true; }
		bool await_ready() { return m_ready; }
		void await_suspend(std::coroutine_handle<> awaiting) { m_awaiting = awaiting; }
		auto await_resume() { return m_coroutine ? m_coroutine.promise().value : 0; }
		void destroy();

	private:
		std::coroutine_handle<promise_type> m_coroutine = nullptr;
		std::coroutine_handle<> m_awaiting = nullptr;
		bool m_ready = false;
	};
}