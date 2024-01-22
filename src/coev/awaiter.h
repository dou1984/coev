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
#include "chain.h"
#include "promise.h"
#include "log.h"
#include "object.h"
#include "taskevent.h"

namespace coev
{
	class awaiter final : public taskevent
	{
	public:
		struct promise_type : promise
		{
			int value;
			awaiter *m_awaiter = nullptr;
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
		bool done();
		bool await_ready();	
		void await_suspend(std::coroutine_handle<> awaiting);
		auto await_resume()	{ return m_coroutine ? m_coroutine.promise().value : 0; }		
		void destroy();

	private:
		std::coroutine_handle<promise_type> m_coroutine = nullptr;
		std::coroutine_handle<> m_awaiting = nullptr;		
		std::atomic_int m_state {CONSTRUCT};
	};
}