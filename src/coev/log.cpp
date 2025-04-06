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
	const char *g_log_str[] = {
		"CORE",
		"DEBUG",
		"WARN",
		"ERROR",
		"INFO"};
	int set_log_level(int _level)
	{
		g_log_level = _level;
		return g_log_level;
	}
	int get_log_level()
	{
		return g_log_level;
	}
	std::mutex &get_log_mutex()
	{
		static std::mutex _;
		return _;
	}
	const char *get_log_str(int level)
	{
		return g_log_str[level];
	}
}
