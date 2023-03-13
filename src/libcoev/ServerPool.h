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
#include "Server.h"

namespace coev
{
	class ServerPool final
	{
		int m_fd = INVALID;
		std::array<Server, 0x40> m_pool;
		std::mutex m_mutex;

	public:
		operator Server &();
		int start(const char *, int);
		int stop();
	};

}