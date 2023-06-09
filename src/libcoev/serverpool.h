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
		std::array<server, max_ev_loop> m_pool;
		std::mutex m_mutex;

	public:
		server& get();
		int start(const char *, int);
		int stop();
	};

}