/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <sstream>
#include <cosys/cosys.h>
#include <co_http/Httprequest.h>
#include <co_http/HttpServer.h>

using namespace coev;

server_pool<HttpServer> g_server;

awaitable<int> echo(io_context &io)
{
	LOG_DBG("recv echo\n");
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
		LOG_ERR("send error %s\n", strerror(errno));
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
			LOG_CORE("accept error %s\n", strerror(errno));
			continue;
		}
		[=]() -> awaitable<void>
		{
			io_context io(fd);
			HttpRequest r;
			auto [ok, req] = co_await r.get_request(io);
			if (ok != 0)
			{
				LOG_DBG("parse http request error %s\n", req.status.c_str());
				co_return;
			}
			std::cout << "url:" << req.url << std::endl;
			for (auto &it : req.headers)
			{
				std::cout << "header:" << it.first << ":" << it.second << std::endl;
			}
			std::cout << "body:" << req.body << std::endl;

			co_await echo(io);

			io.close();
			LOG_DBG("close socket %d\n", fd);
		}();
	}
}
int main()
{
	g_server.start("0.0.0.0", 9999);

	set_log_level(LOG_LEVEL_DEBUG);
	running::instance()
		.add(1, co_router)
		.join();
	return 0;
}
