/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <cstdlib>
#include "../coev.h"

namespace coev
{
	template <class... TYPE>
	void unpack(MYSQL_ROW rows, TYPE &...value)
	{
		int i = 0;
		(__SetValue(value, rows[i++]), ...);
	}
}