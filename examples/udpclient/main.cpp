/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev/coev.h>

using namespace coev;

awaitable<void> go()
{
	addrInfo addr("127.0.0.1", 9998);

	auto fd = udp::socketfd();
	io_context io(fd);
	while (io)
	{
		char buffer[1000] = "hello world";
		co_await io.sendto(buffer, strlen(buffer) + 1, addr);
		LOG_DBG("send to %s:%d %s\n", addr.ip, addr.port, buffer);
		co_await io.recvfrom(buffer, sizeof(buffer), addr);
		LOG_DBG("recv from %s:%d %s\n", addr.ip, addr.port, buffer);
	}
}
int main()
{

	runnable::instance().add(go).join();
	return 0;
}