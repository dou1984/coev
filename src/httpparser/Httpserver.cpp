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
	awaitable<void> Httpserver::parse(iocontext &io, Httpparser &req)
	{
		while (io)
		{
			char buffer[0x1000];
			auto r = co_await io.recv(buffer, sizeof(buffer));
			if (r == INVALID)
			{
				break;
			}
			req.parse(buffer, r);
		}
	}

	Httpserver::Httpserver(const char *ip, int port)
	{
		m_pool.start(ip, port);
	}

	awaitable<int> Httpserver::accept(host &h)
	{
		return m_pool.get().accept(h);
	}
}