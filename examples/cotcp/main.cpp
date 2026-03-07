/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <atomic>
#include <coev/coev.h>

using namespace coev;
using namespace coev::pool;

coev::pool::server_pool<coev::tcp::server> _pool;
coev::pool::tcp::client _cpool;

std::atomic_int g_count = {0};

std::string host = "0.0.0.0";
uint16_t port = 9999;
int max_co_client = 100;
int max_send_count = 1000000;
int max_runner = 4;
int max_connections = 10;

co_waitgroup wg;
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
	auto &srv = _pool.get();
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

awaitable<int> co_dail()
{
	defer(wg.done());

	char sayhi[64] = "helloworld";
	int count = 0;
	LOG_DBG("co_dail start %s %d", sayhi, port);
	while (true)
	{
		coev::pool::tcp::client::instance c;
		auto err = co_await _cpool.get(c);
		if (err != 0)
		{
			LOG_DBG("co_dail co_await _cpool.get()");
			co_return INVALID;
		}
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
		if (count++ >= max_send_count)
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
	wg.add(max_co_client);
	for (int i = 0; i < max_co_client; i++)
	{
		co_start << co_dail();
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

	set_log_level(LOG_LEVEL_ERROR);
	if (method == "server")
	{
		_pool.start(host.c_str(), port);
		runnable::instance()
			.start(max_runner, co_server)
			.end(
				[]()
				{
					_pool.stop();
					LOG_DBG("g_count %d", g_count.load());
				});
	}
	else if (method == "client")
	{

		auto config = _cpool.get_config();
		config->max_connections = max_connections;
		config->retry_time = 1.0f;
		config->quick_retry_time = 0.1f;
		config->host = host;
		config->port = port;
		_cpool.set(config);

		runnable::instance()
			.start(max_runner, co_client)
			.end(
				[]()
				{
					_cpool.stop();
					LOG_DBG("g_count %d", g_count.load());
				});
	}
	else
	{
		LOG_ERR("invalid method");
	}
	return 0;
}