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

ServerPool pool;

Task echo(IOContext &c, Httprequest &req)
{
	co_await wait_for<EVRecv>(req);
	std::ostringstream oss;
	oss << R"(HTTP/1.1 200 OK
Date: Sun, 17 Mar 2017 08:12:54 GMT
Server: Apache/2.2.8 (Win32) PHP/5.2.5
X-Powered-By: PHP/5.2.5
Set-Cookie: PHPSESSID=c0huq7pdkmm5gg6osoe3mgjmm3; path=/
Expires: Thu, 19 Nov 1981 08:52:00 GMT
Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0
Pragma: no-cache
Content-Length: 0
Keep-Alive: timeout=5, max=100
Connection: Keep-Alive
Content-Type: text/html; charset=utf-8)";
	auto s = oss.str();
	co_await send(c, s.data(), s.size());
	TRACE();
	co_return 0;
}
Task get_request(IOContext &c, Httprequest &req)
{
	while (c)
	{
		char buffer[0x1000];
		auto r = co_await recv(c, buffer, sizeof(buffer));
		if (r == INVALID)
		{
			close(c);
			co_return r;
		}
		req.parse(buffer, r);
	}
	co_return 0;
}
Awaiter<int> dispatch(int fd)
{
	IOContext c(fd);
	Httprequest req;
	co_await wait_for_any(get_request(c, req), echo(c, req));
	co_return 0;
}
Awaiter<int> co_httpserver()
{
	Server &s = pool;
	while (s)
	{
		ipaddress addr;
		auto fd = co_await accept(s, addr);
		if (fd != INVALID)		
			dispatch(fd);		
	}
	co_return INVALID;
}

int main()
{
	ingore_signal(SIGPIPE);
	set_log_level(LOG_LEVEL_ERROR);

	pool.start("127.0.0.1", 9960);
	Routine r;
	r.add(co_httpserver);
	r.add(co_httpserver);
	r.add(co_httpserver);
	r.add(co_httpserver);
	Loop::start();
	return 0;
}