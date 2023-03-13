/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <stdio.h>
#include <string.h>

#define PRINT(...) printf(__VA_ARGS__)
#define LOG(LEVEL, ...)                   \
	if (get_log_level() <= LEVEL)         \
	{                                     \
		auto f = strrchr(__FILE__, '/');  \
		f = f ? f + 1 : __FILE__;         \
		PRINT("[%s:%d/%s]",               \
			  f, __LINE__, __FUNCTION__); \
		PRINT(__VA_ARGS__);               \
	}
#define LOG_CORE(...) \
	LOG(LOG_LEVEL_CORE, __VA_ARGS__)
#define LOG_DBG(...) \
	LOG(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_WARN(...) \
	LOG(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_ERR(...) \
	LOG(LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_FATAL(...) \
	LOG(LOG_LEVEL_FATAL, __VA_ARGS__)

#define TRACE() LOG_CORE("\n")

namespace coev
{
	enum LOG_LEVEL
	{
		LOG_LEVEL_CORE = 0,
		LOG_LEVEL_DEBUG,
		LOG_LEVEL_WARN,
		LOG_LEVEL_ERROR,
		LOG_LEVEL_FATAL,
	};
	int set_log_level(int);
	int get_log_level();
}