/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include "chain.h"
#include "eventchain.h"

namespace coev
{
	struct task;

	class taskext : protected chain
	{
	protected:
		friend struct task;

		task *m_taskchain = nullptr;
		void __resume();
		virtual ~taskext();
	};

}