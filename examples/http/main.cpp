/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <sstream>
#include <coev/coev.h>
#include <coev_http/session.h>
#include <coev_http/server.h>

using namespace coev;
using namespace coev::pool;

server_pool<http::server> g_server;

awaitable<void> echo(io_context &io, http::request &req)
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
	int err = co_await io.send(response);
	if (err == INVALID)
	{
		LOG_ERR("send error %s", strerror(errno));
	}
}

awaitable<void> co_router()
{
	auto &srv = g_server.get();
	srv.set_router("/echo", echo);
	co_await srv.worker();
}
int main()
{
	g_server.start("0.0.0.0", 9999);

	set_log_level(LOG_LEVEL_CORE);
	runnable::instance()
		.start(1, co_router)
		.end([&]()
			 { g_server.stop(); });
	return 0;
}
