/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev.h>

using namespace coev;

tcp::server srv;
awaiter<int> dispatch(const ipaddress &addr, iocontext &io)
{
	while (io)
	{
		char buffer[0x1000];
		auto r = co_await io.recv(buffer, sizeof(buffer));
		if (r == INVALID)
		{
			co_await io.close();
			co_return 0;
		}
		LOG_DBG("%s\n", buffer);
		r = co_await io.send(buffer, r);
		if (r == INVALID)
		{
			co_await io.close();
			co_return 0;
		}
	}
	co_return INVALID;
}
awaiter<int> co_server()
{
	while (srv)
	{
		co_await srv.accept(dispatch);
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