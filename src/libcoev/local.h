/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <memory>
#include <atomic>
#include "event.h"
#include "task.h"

namespace coev
{
	struct localext;
	struct local : EVRecv
	{
		std::atomic<int> m_ref{0};
		static std::unique_ptr<localext> ref();
	};
	struct localext
	{
		std::shared_ptr<local> m_current = nullptr;
		localext(std::shared_ptr<local> &_);
		localext(localext &&) = delete;
		~localext();
	};

	awaiter<int> wait_for_local();
}