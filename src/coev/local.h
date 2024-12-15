/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <unordered_map>

namespace coev
{
	template <class T>
	struct local final
	{
		static T &instance()
		{
			static thread_local T obj;
			return obj;
		}
	};
}
