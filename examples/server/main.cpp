/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev/coev.h>

using namespace coev;

server_pool<tcp::server> pool;
awaitable<void> dispatch(addrInfo addr, int fd)
{
	LOG_DBG("dispatch start %s %d\n", addr.ip, addr.port);
	defer _([=]()
			{ LOG_DBG("dispatch exit %s %d\n", addr.ip, addr.port); });
	io_context io(fd);
	while (io)
	{
		char buffer[0x1000];
		auto r = co_await io.recv(buffer, sizeof(buffer));
		if (r == INVALID)
		{
			io.close();
			co_return;
		}
		LOG_DBG("recv %d %s\n", r, buffer);
		r = co_await io.send(buffer, r);
		if (r == INVALID)
		{
			io.close();
			co_return;
		}
		LOG_DBG("send %d %s\n", r, buffer);
	}
}
awaitable<int> co_server()
{
	auto &srv = pool.get();
	addrInfo h;
	while (true)
	{
		auto fd = co_await srv.accept(h);
		if (srv.valid())
		{
			dispatch(h, fd);
		}
	}
	co_return INVALID;
}

int main()
{

	set_log_level(LOG_LEVEL_CORE);

	pool.start("0.0.0.0", 9999);

	running::instance().add(1, co_server).join();
	return 0;
}