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
	struct LocalExt;
	struct Local : EVRecv
	{
		std::atomic<int> m_ref{0};
		static std::unique_ptr<LocalExt> ref();
	};
	struct LocalExt
	{
		std::shared_ptr<Local> m_current = nullptr;
		LocalExt(std::shared_ptr<Local> &_);
		LocalExt(LocalExt &&) = delete;
		~LocalExt();
	};

	awaiter<int> wait_for_local();
}