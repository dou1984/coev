/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <coroutine>
#include "Log.h"
#include "Chain.h"
#include "Promise.h"

namespace coev
{
	struct Event final : Chain
	{
		Chain *m_object = nullptr;
		std::coroutine_handle<> m_awaiting = nullptr;
		uint32_t m_tag = 0;
		Event(Chain *obj, uint32_t _tag);
		Event(Chain* obj);
		~Event();
		Event(Event &&) = delete;
		void await_resume();
		bool await_ready();
		void await_suspend(std::coroutine_handle<> awaiting);
		void resume_ex();
	};
}
