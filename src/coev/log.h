/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <stdio.h>
#include <string.h>
#include <mutex>

#define PRINT(...) printf(__VA_ARGS__)
#define LOG(LEVEL, ...)                                               \
	if (coev::get_log_level() <= LEVEL)                               \
	{                                                                 \
		std::lock_guard<std::mutex> _(coev::get_log_mutex());         \
		coev::print_log_str(LEVEL, __FILE__, __FUNCTION__, __LINE__); \
		PRINT(__VA_ARGS__);                                           \
	}

#define LOG_CORE(...) \
	LOG(coev::LOG_LEVEL_CORE, __VA_ARGS__)
#define LOG_DBG(...) \
	LOG(coev::LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_WARN(...) \
	LOG(coev::LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_ERR(...) \
	LOG(coev::LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_INFO(...) \
	LOG(coev::LOG_LEVEL_INFO, __VA_ARGS__)
#define TRACE() LOG_CORE("\n")

namespace coev
{
	enum LOG_LEVEL
	{
		LOG_LEVEL_CORE = 0,
		LOG_LEVEL_DEBUG,
		LOG_LEVEL_WARN,
		LOG_LEVEL_ERROR,
		LOG_LEVEL_INFO,
	};

	void print_log_str(int, const char *, const char *, size_t);
	int set_log_level(int);
	int get_log_level();
	std::mutex &get_log_mutex();
	const char *get_log_str(int _level);
}