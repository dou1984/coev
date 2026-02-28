/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <atomic>
#include <coev/coev.h>

using namespace coev;

server_pool<tcp::server> pool;
client_pool<io_connect> cpool;

std::atomic_int g_count = {0};

std::string host = "0.0.0.0";
uint16_t port = 9999;
int max_co_client = 1;
awaitable<void> dispatch(addrInfo addr, int fd)
{
	LOG_DBG("dispatch start %s %d", addr.ip, addr.port);
	io_context io(fd);
	defer(io.close(););
	while (io)
	{
		char buffer[0x1000];
		auto r = co_await io.recv(buffer, sizeof(buffer));
		if (r == INVALID)
		{
			co_return;
		}
		LOG_DBG("recv %d %s", r, buffer);
		r = co_await io.send(buffer, r);
		if (r == INVALID)
		{
			co_return;
		}
		LOG_DBG("send %d %s", r, buffer);
		g_count++;
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
			if (fd != INVALID)
			{
				co_start << dispatch(h, fd);
			}
		}
	}
}

awaitable<int> co_dail(co_waitgroup &wg)
{
	wg.add(1);
	defer(wg.done());
	defer(LOG_CORE("wg.done()"));
	auto c = co_await cpool.get();
	if (!c)
	{
		LOG_DBG("co_dail co_await cpool.get()");
		co_return INVALID;
	}

	char sayhi[64] = "helloworld";
	int count = 0;
	LOG_DBG("co_dail start %s %d", sayhi, port);
	while (c)
	{
		LOG_DBG("co_dail send %d", count);
		int r = co_await c->send(sayhi, strlen(sayhi) + 1);
		if (r == INVALID)
		{
			co_return 0;
		}
		LOG_DBG("send %d %s", r, sayhi);
		char buffer[0x1000];
		r = co_await c->recv(buffer, sizeof(buffer));
		if (r == INVALID)
		{
			co_return 0;
		}
		LOG_DBG("recv %d %s", r, buffer);
		if (count++ >= 10)
		{
			LOG_DBG("co_dail exit %s %d %d", host.c_str(), port, count);
			co_return 0;
		}
		g_count++;
	}
	co_return 0;
}

awaitable<void> co_client()
{
	co_waitgroup wg;
	for (int i = 0; i < max_co_client; i++)
	{
		co_start << co_dail(wg);
	}
	co_await wg.wait();
	LOG_DBG("co_client exit");
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
			.start(4, co_server)
			.endless(
				[]()
				{
					pool.stop();
					LOG_DBG("g_count %d", g_count.load());
				});
	}
	else if (method == "client")
	{

		cpool.set({
			.m_max = 4,
			.m_delay = 1.0f,
			.m_short_delay = 0.1f,
			.m_host = host,
			.m_port = port,
		});

		runnable::instance()
			.start(1, co_client)
			.endless(
				[]()
				{
					cpool.stop();
					LOG_DBG("g_count %d", g_count.load());
				});
	}
	else
	{
		LOG_ERR("invalid method");
	}
	return 0;
}