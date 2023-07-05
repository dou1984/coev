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

	class taskext : public chain
	{
	public:
		task *m_taskchain = nullptr;
		virtual void destroy() = 0;

	protected:
		void __resume();
		virtual ~taskext();
	};

}