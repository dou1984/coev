/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#pragma once

namespace coev
{
	template <class _Type>
	class singleton
	{
	public:
		static _Type &instance()
		{
			static _Type obj;
			return obj;
		}
	};
}