/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "log.h"

namespace coev
{
	int g_log_level = LOG_LEVEL_DEBUG;
	int set_log_level(int _level)
	{
		g_log_level = _level;
		return g_log_level;
	}
	int get_log_level()
	{
		return g_log_level;
	}
}