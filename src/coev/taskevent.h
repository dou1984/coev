/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <functional>
#include "chain.h"

namespace coev
{
	class task;
	struct taskevent : chain
	{
		task *m_task = nullptr;
		virtual ~taskevent();	
		virtual void destroy();
		void resume();
	};

}