/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev.h>

using namespace coev;

awaiter<int> go()
{
	auto c = udp::bind("127.0.0.1", 9998);

	while (c)
	{
		ipaddress addr;
		char buffer[1000];
		auto r = co_await c.recvfrom(buffer, sizeof(buffer), addr);
		LOG_DBG("recvfrom %s:%d %s\n", addr.ip, addr.port, buffer);
		co_await c.sendto(buffer, r, addr);
		LOG_DBG("sendto %s:%d %s\n", addr.ip, addr.port, buffer);
	}

	co_return 0;
}
int main()
{

	routine::instance().add(go);
	routine::instance().join();
	return 0;
}