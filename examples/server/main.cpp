/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev.h>

using namespace coev;

ServerPool pool;

awaiter<int> dispatch(SharedIO io)
{
	auto& c = *io;
	while (c)
	{
		char buffer[0x1000];
		auto r = co_await recv(c, buffer, sizeof(buffer));
		if (r == INVALID)
		{
			close(c);
			co_return 0;
		}
		LOG_DBG("%s\n", buffer);
		r = co_await send(c, buffer, r);
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
	Server& s = pool;
	while (s)
	{
		ipaddress addr;
		auto io = co_await accept(s, addr);
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
	set_log_level(LOG_LEVEL_ERROR);

	pool.start("127.0.0.1", 9960);
	routine r;
	r.add(co_server);
	r.add(co_server);
	r.add(co_server);
	r.add(co_server);
	coev::loop::start();
	return 0;
}