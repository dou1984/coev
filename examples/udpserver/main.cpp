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

awaitable<int> go(int fd)
{
	io_context io(fd);
	while (io)
	{
		addrInfo addr;
		char buffer[1000];
		auto r = co_await io.recvfrom(buffer, sizeof(buffer), addr);
		LOG_DBG("recvfrom %s:%d %s\n", addr.ip, addr.port, buffer);
		co_await io.sendto(buffer, r, addr);
		LOG_DBG("sendto %s:%d %s\n", addr.ip, addr.port, buffer);
	}

	co_return 0;
}
int main()
{
	auto fd = udp::bindfd("127.0.0.1", 9998);
	running::instance()
		.add(4,
			 [=]()
			 { go(fd); })
		.join();

	return 0;
}