/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "../cosys/coloop.h"

using namespace coev;

tcp::serverpool pool;
awaitable<int> dispatch(const ipaddress &addr, iocontext &io)
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
awaitable<int> co_server()
{
	co_await pool.get().accept(dispatch);
	co_return INVALID;
}

int main()
{

	set_log_level(LOG_LEVEL_ERROR);

	pool.start("127.0.0.1", 9960);

	running::instance().add(4, co_server).join();
	return 0;
}