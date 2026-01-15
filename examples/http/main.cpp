/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <sstream>
#include <coev/coev.h>
#include <coev_http/session.h>
#include <coev_http/server.h>

using namespace coev;

server_pool<http::server> g_server;

awaitable<int> echo(io_context &io)
{
	LOG_DBG("reach echo");
	std::ostringstream oss;
	oss << R"(HTTP/1.1 200 OK
Date: Sun, 26 OCT 1984 10:00:00 GMT
Server: coev
Set-Cookie: COEV=0000000000000000`; path=/
Expires: Sun, 26 OCT 2100 10:00:00 GMT
Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0
Pragma: no-cache
Content-Length: 0
Keep-Alive: timeout=5, max=100
Connection: Keep-Alive
Content-Type: text/html; charset=utf-8)";

	auto response = oss.str();
	int err = co_await io.send(response.data(), response.size());
	if (err == INVALID)
	{
		LOG_ERR("send error %s", strerror(errno));
	}
	co_return 0;
}

awaitable<void> co_router()
{
	auto &srv = g_server.get();
	while (srv.valid())
	{
		addrInfo h;
		auto fd = co_await srv.accept(h);
		if (fd == INVALID)
		{
			LOG_CORE("accept error %s", strerror(errno));
			continue;
		}
		co_start << [fd, h]() -> awaitable<void>
		{
			io_context io(fd);
			http::session r;
			http::request req;
			auto ok = co_await r.get_request(io, req);
			if (ok != 0)
			{
				LOG_DBG("parse http request error %s", req.status.c_str());
				co_return;
			}
			LOG_DBG("url:%s", req.url.c_str());
			for (auto &it : req.headers)
			{
				std::cout << "header:" << it.first << ":" << it.second << std::endl;
			}
			LOG_DBG("body:%ld", req.body.size());

			co_await echo(io);
			LOG_DBG("echo end");

			co_return;
		}();
	}
}
int main()
{
	g_server.start("0.0.0.0", 9999);

	set_log_level(LOG_LEVEL_CORE);
	runnable::instance()
		.start(1, co_router)
		.wait();
	return 0;
}
