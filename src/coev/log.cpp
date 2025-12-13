/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <chrono>
#include <ctime>
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
	const char *get_log_str(int _level)
	{
		return g_log_str[_level];
	}
	void print_log_str(int _level, const char *_file, const char *_func, size_t _line)
	{
		auto __now__ = std::chrono::system_clock::now();
		auto __count__ =
			std::chrono::duration_cast<std::chrono::nanoseconds>(
				__now__.time_since_epoch())
				.count();
		auto __tm__ = std::chrono::system_clock::to_time_t(__now__);
		auto __lt__ = std::localtime(&__tm__);
		auto __fl__ = strrchr(_file, '/');
		__fl__ = __fl__ ? __fl__ + 1 : _file;
		PRINT("[%d/%d/%d %02d:%02d:%02d.%ld %s][%s:%ld][%s]",
			  __lt__->tm_year + 1900,
			  __lt__->tm_mon + 1,
			  __lt__->tm_mday,
			  __lt__->tm_hour,
			  __lt__->tm_min,
			  __lt__->tm_sec,
			  __count__ % 1000000000,
			  get_log_str(_level),
			  __fl__, _line, _func);
	}
}
