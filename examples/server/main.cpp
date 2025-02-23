/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev/coev.h>
#include <cosys/cosys.h>

using namespace coev;

tcp::serverpool<tcp::server> pool;
awaitable<void> dispatch(addrInfo addr, int fd)
{
	io_context io(fd);
	while (io)
	{
		char buffer[0x1000];
		auto r = co_await io.recv(buffer, sizeof(buffer));
		if (r == INVALID)
		{
			io.close();
		}
		LOG_DBG("%s\n", buffer);
		r = co_await io.send(buffer, r);
		if (r == INVALID)
		{
			io.close();
		}
	}
}
awaitable<int> co_server()
{
	auto &srv = pool.get();
	addrInfo h;
	auto fd = co_await srv.accept(h);
	if (srv.valid())
	{
		dispatch(h, fd);
	}
	co_return INVALID;
}

int main()
{

	set_log_level(LOG_LEVEL_ERROR);

	pool.start("127.0.0.1", 9960);

	running::instance().add(4, co_server).join();
	return 0;
}