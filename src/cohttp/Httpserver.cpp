/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "Httpserver.h"

namespace coev
{
	Httpserver::Httpserver(const char *ip, int port)
	{
		m_pool.start(ip, port);
	}

	awaitable<int> Httpserver::accept(host &h)
	{
		return m_pool.get().accept(h);
	}
}