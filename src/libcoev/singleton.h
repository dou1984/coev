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
	template <class _Type>
	struct singleton final
	{
		static _Type &instance()
		{
			static _Type obj;
			return obj;
		}
	};
}