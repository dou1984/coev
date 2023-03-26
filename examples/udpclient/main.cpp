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
	ipaddress addr = {"127.0.0.1", 9998};
	auto c = Udp::socket();
	while (c)
	{
		char buffer[1000] = "hello world";
		co_await sendto(c, buffer, strlen(buffer) + 1, addr);
		LOG_DBG("send to %s:%d %s\n", addr.ip, addr.port, buffer);
		co_await recvfrom(c, buffer, sizeof(buffer), addr);
		LOG_DBG("recv from %s:%d %s\n", addr.ip, addr.port, buffer);
	}
}
int main()
{

	Routine r;
	r.add(go);
	r.join();
	return 0;
}