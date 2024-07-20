/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include "chain.h"

namespace coev
{
	struct task;

	struct taskevent : chain
	{
		friend struct task;

		task *m_taskchain = nullptr;
		void __resume();
		virtual ~taskevent();
		virtual void destroy();
	};

}