/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <thread>
#include "gtid.h"

namespace coev
{
	uint64_t gtid()
	{
		thread_local std::hash<std::thread::id> hasher;
		thread_local auto _id = hasher(std::this_thread::get_id());
		return _id;
	}
}