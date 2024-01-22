/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once

namespace coev
{
	struct OBJECT0
	{
	};
	struct OBJECT1
	{
	};
	
	struct FMUTEX
	{
		void lock() {}
		void unlock() {}
	};
}