#pragma once
#include <cstdlib>
#include "../coev.h"

namespace coev
{
	template <class... TYPE>
	void unpack(MYSQL_ROW rows, TYPE &...value)
	{
		int i = 0;
		(__SetValue(rows[i++], value), ...);
	}
}