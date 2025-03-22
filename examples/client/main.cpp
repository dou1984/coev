/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev/coev.h>
#include <coev/coev.h>
using namespace coev;

awaitable<int> co_dail(const char *ip, int port)
{
	io_connect c;
	co_await c.connect(ip, port);
	if (!c)
	{
		co_return INVALID;
	}
	char sayhi[] = "helloworld";
	int count = 0;
	LOG_DBG("co_dail start %s %d\n", sayhi, port);
	defer _([=]()
			{ LOG_DBG("co_dail exit %s %d\n", ip, port); });
	while (c)
	{
		auto r = co_await c.send(sayhi, strlen(sayhi) + 1);
		if (r == INVALID)
		{
			c.close();
			co_return 0;
		}
		LOG_DBG("send %s\n", sayhi);
		char buffer[0x1000];
		r = co_await c.recv(buffer, sizeof(buffer));
		if (r == INVALID)
		{
			c.close();
			co_return 0;
		}
		LOG_DBG("recv %s\n", buffer);
		if (count++ > 2)
		{
			LOG_DBG("co_dail exit %s %d %d\n", ip, port, count);
			co_return 0;
		}
	}
	co_return 0;
}
void co_test()
{
	for (int i = 0; i < 1; i++)
	{
		co_dail("0.0.0.0", 9999);
	}
}
int main()
{

	set_log_level(LOG_LEVEL_CORE);
	running::instance().add(1, co_test).join();

	return 0;
}