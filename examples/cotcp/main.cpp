/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <coev/coev.h>

using namespace coev;

server_pool<tcp::server> pool;
thread_local client_pool<io_connect> cpool;

std::string host = "0.0.0.0";
int port = 9999;
awaitable<void> dispatch(addrInfo addr, int fd)
{
	LOG_DBG("dispatch start %s %d", addr.ip, addr.port);
	io_context io(fd);
	while (io)
	{
		char buffer[0x1000];
		auto r = co_await io.recv(buffer, sizeof(buffer));
		if (r == INVALID)
		{
			io.close();
			co_return;
		}
		LOG_DBG("recv %d %s", r, buffer);
		r = co_await io.send(buffer, r);
		if (r == INVALID)
		{
			io.close();
			co_return;
		}
		LOG_DBG("send %d %s", r, buffer);
	}
}
awaitable<void> co_server()
{
	auto &srv = pool.get();
	addrInfo h;
	while (true)
	{
		auto fd = co_await srv.accept(h);
		if (srv.valid())
		{
			co_start << dispatch(h, fd);
		}
	}
}

awaitable<int> co_dail()
{
	auto c = co_await cpool.get();
	if (!c)
	{
		co_return INVALID;
	}
	if (c->__invalid())
	{
		co_await c->connect(host.c_str(), port);
		if (!c)
		{
			co_return INVALID;
		}
	}
	char sayhi[] = "helloworld";
	int count = 0;
	LOG_DBG("co_dail start %s %d", sayhi, port);
	while (c->__valid())
	{
		LOG_DBG("co_dail send %d", count);
		int r = co_await c->send(sayhi, strlen(sayhi) + 1);
		if (r == INVALID)
		{
			c->close();
			co_return 0;
		}
		LOG_DBG("send %d %s", r, sayhi);
		char buffer[0x1000];
		r = co_await c->recv(buffer, sizeof(buffer));
		if (r == INVALID)
		{
			c->close();
			co_return 0;
		}
		LOG_DBG("recv %d %s", r, buffer);
		if (count++ > 10)
		{
			LOG_DBG("co_dail exit %s %d %d", ip, port, count);
			co_return 0;
		}
	}
	co_return 0;
}
awaitable<void> co_test()
{
	for (int i = 0; i < 100; i++)
	{
		co_start << co_dail();
	}
	co_return;
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
		LOG_INFO("Usage: %s <host> <port>", argv[0])
		exit(1);
	}

	set_log_level(LOG_LEVEL_CORE);
	if (method == "server")
	{
		pool.start(host.c_str(), port);
		runnable::instance()
			.start(2, co_server)
			.endless([]()
					 { pool.stop(); });
	}
	else if (method == "client")
	{
		runnable::instance()
			.start(4, co_test)
			.wait();
	}
	else
	{
		LOG_ERR("invalid method");
	}
	return 0;
}