#pragma once
#include "../coev.h"

namespace coev
{
	template <class... TYPE>
	void unpack(redisReply** rows, TYPE &...value)
	{
		int i = 0;
		(__SetValue(rows[i++]->str, value), ...);
	}
}