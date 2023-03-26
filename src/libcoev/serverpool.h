/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <array>
#include <mutex>
#include "server.h"

namespace coev::tcp
{
	class serverpool final
	{
		int m_fd = INVALID;
		std::array<server, 0x40> m_pool;
		std::mutex m_mutex;

	public:
		operator server &();
		int start(const char *, int);
		int stop();
	};

}