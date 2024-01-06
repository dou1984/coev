/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <ctime>
#include <mutex>

#define PRINT(...) printf(__VA_ARGS__)
#define LOG(LEVEL, ...)                                                  \
	if (get_log_level() <= LEVEL)                                        \
	{                                                                    \
		std::lock_guard<std::mutex> _(get_log_mutex());                  \
		{                                                                \
			auto __now__ = std::chrono::system_clock::now();             \
			auto __tm__ = std::chrono::system_clock::to_time_t(__now__); \
			auto __lt__ = std::localtime(&__tm__);                       \
			auto __fl__ = strrchr(__FILE__, '/');                        \
			__fl__ = __fl__ ? __fl__ + 1 : __FILE__;                     \
			PRINT("[%d/%d/%d %02d:%02d:%02d %s][%s:%d/%s]",              \
				  __lt__->tm_year + 1900,                                \
				  __lt__->tm_mon + 1,                                    \
				  __lt__->tm_mday,                                       \
				  __lt__->tm_hour,                                       \
				  __lt__->tm_min,                                        \
				  __lt__->tm_sec,                                        \
				  get_log_str(LEVEL),                                    \
				  __fl__, __LINE__, __FUNCTION__);                       \
		}                                                                \
		{                                                                \
			PRINT(__VA_ARGS__);                                          \
		}                                                                \
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
	std::mutex &get_log_mutex();
	const char *get_log_str(int level);
}