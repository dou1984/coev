/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev.h>

using namespace coev;

Awaiter<int> co_dail(const char *ip, int port)
{
	Client c;
	co_await connect(c, ip, port);
	if (!c)
	{
		co_return INVALID;
	}
	char sayhi[] = "helloworld";
	auto r = co_await send(c, sayhi, sizeof(sayhi));
	if (r == INVALID)
	{
		co_await close(c);
		co_return 0;
	}
	int count = 0;
	while (c)
	{
		char buffer[0x1000];
		r = co_await recv(c, buffer, sizeof(buffer));
		if (r == INVALID)
		{
			LOG_ERR("close %d\n", c.m_fd);
			co_await close(c);
			co_return 0;
		}
		LOG_DBG("%s\n", buffer);
		r = co_await send(c, buffer, r);
		if (r == INVALID)
		{
			co_await close(c);
			co_return 0;
		}
		if (count++ > 100000)
		{
			co_return 0;
		}
	}
	LOG_FATAL("finish %d\n", c.m_fd);
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
	ingore_signal(SIGPIPE);
	set_log_level(LOG_LEVEL_ERROR);

	Routine r;
	r.add(co_test);
	r.add(co_test);
	r.add(co_test);
	r.add(co_test);
	r.join();

	return 0;
}