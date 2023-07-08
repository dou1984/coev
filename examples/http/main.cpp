/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <sstream>
#include <coev.h>
#include <coapp.h>

using namespace coev;

tcp::server srv;

awaiter<int> echo(iocontext &c, Httprequest &req)
{
	co_await wait_for<EVRecv>(req);
	std::ostringstream oss;
	oss << R"(HTTP/1.1 200 OK
Date: Sun, 26 OCT 1984 10:00:00 GMT
Server: coev
Set-Cookie: COEV=0000000000000000`; path=/
Expires: Sun, 26 OCT 2022 10:00:00 GMT
Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0
Pragma: no-cache
Content-Length: 0
Keep-Alive: timeout=5, max=100
Connection: Keep-Alive
Content-Type: text/html; charset=utf-8)";
	auto s = oss.str();
	co_await c.send(s.data(), s.size());
	co_return 0;
}
awaiter<int> get_request(iocontext &io, Httprequest &req)
{
	while (io)
	{
		char buffer[0x1000];
		auto r = co_await io.recv(buffer, sizeof(buffer));
		if (r == INVALID)
		{
			co_await io.close();
			co_return r;
		}
		req.parse(buffer, r);
	}
	co_return 0;
}
awaiter<int> dispatch(const ipaddress &addr, iocontext &io)
{
	Httprequest req;
	co_await wait_for_any(get_request(io, req), echo(io, req));
	co_return 0;
}
awaiter<int> co_httpserver()
{
	while (srv)
	{
		co_await srv.accept(dispatch);
	}
	co_return INVALID;
}

int main()
{
	ingore_signal(SIGPIPE);
	set_log_level(LOG_LEVEL_ERROR);

	srv.start("127.0.0.1", 9960);
	routine r;
	r.add(co_httpserver);
	loop::start();
	return 0;
}