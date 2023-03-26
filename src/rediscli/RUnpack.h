/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include "../coev.h"

namespace coev
{
	template <class... TYPE>
	void unpack(redisReply **rows, TYPE &...value)
	{
		int i = 0;
		(__setvalue(value, rows[i++]->str), ...);
	}
}