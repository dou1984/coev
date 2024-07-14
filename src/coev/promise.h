/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <coroutine>
#include <iostream>
#include <string.h>
#include "log.h"

namespace coev::details
{
	enum status : int
	{
		STATUS_INIT,
		STATUS_SUSPEND,
		STATUS_READY,
	};
	
	struct awaiter_impl;
	struct promise
	{
		awaiter_impl *m_awaiter = nullptr;

		promise() = default;
		~promise();
		void unhandled_exception();
		std::suspend_never initial_suspend();
		std::suspend_never final_suspend() noexcept;
	};

}
