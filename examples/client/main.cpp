/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coloop.h>

using namespace coev;

awaiter co_dail(const char *ip, int port)
{
	client c;
	co_await c.connect(ip, port);
	if (!c)
	{
		co_return INVALID;
	}
	char sayhi[] = "helloworld";
	auto r = co_await c.send(sayhi, sizeof(sayhi));
	if (r == INVALID)
	{
		co_await c.close();
		co_return 0;
	}
	int count = 0;
	while (c)
	{
		char buffer[0x1000];
		r = co_await c.recv(buffer, sizeof(buffer));
		if (r == INVALID)
		{
			co_await c.close();
			co_return 0;
		}
		LOG_DBG("%s\n", buffer);
		r = co_await c.send(buffer, r);
		if (r == INVALID)
		{
			co_await c.close();
			co_return 0;
		}
		if (count++ > 100000)
		{
			co_return 0;
		}
	}
	co_return 0;
}
void co_test()
{
	for (int i = 0; i < 1000; i++)
	{
		co_dail("127.0.0.1", 9960);
	}
}
int main()
{

	set_log_level(LOG_LEVEL_ERROR);

	running::instance().add(4, co_test).join();

	return 0;
}