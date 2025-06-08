/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev/coev.h>

using namespace coev;
std::string host = "0.0.0.0";
int port = 9999;
awaitable<int> co_srv(int fd)
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

awaitable<void> co_cli()
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
int main(int argc, char **argv)
{

	std::string method = "server";
	if (argc > 1)
	{
		method = argv[1];
		if (argc > 2)
		{
			host = argv[2];
			if (argc > 3)
			{
				port = atoi(argv[3]);
			}
		}
	}
	else
	{

		LOG_INFO("Usage: %s server|client <host> <port>", argv[0]);
		exit(1);
	}

	set_log_level(LOG_LEVEL_DEBUG);
	if (method == "server")
	{
		runnable::instance()
			.start(4,
				   []() -> awaitable<void>
				   {
					   auto fd = udp::bindfd("127.0.0.1", 9998);
					   co_await co_srv(fd);
				   })
			.join();
	}
	else if (method == "client")
	{
		runnable::instance()
			.start(co_cli)
			.join();
	}
	return 0;
}