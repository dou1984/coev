/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev.h>

using namespace coev;

// tcp::serverpool pool;
tcp::server srv;
awaiter<int> dispatch(sharedIOContext io)
{
	auto &c = *io;
	while (c)
	{
		char buffer[0x1000];
		auto r = co_await c.recv(buffer, sizeof(buffer));
		if (r == INVALID)
		{
			close(c);
			co_return 0;
		}
		LOG_DBG("%s\n", buffer);
		r = co_await c.send(buffer, r);
		if (r == INVALID)
		{
			close(c);
			co_return 0;
		}
	}
	co_return INVALID;
}
awaiter<int> co_server()
{
	// tcp::server &s = pool;
	while (srv)
	{
		ipaddress addr;
		auto io = co_await accept(srv, addr);
		if (*io)
		{
			dispatch(io);
		}
	}
	co_return INVALID;
}

int main()
{
	ingore_signal(SIGPIPE);
	ingore_signal(SIGABRT);
	set_log_level(LOG_LEVEL_ERROR);

	srv.start("127.0.0.1", 9960);
	routine r;
	r.add(co_server);
	coev::loop::start();
	return 0;
}