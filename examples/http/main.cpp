/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <sstream>
#include "../cosys/coapp.h"
#include "../cosys/coloop.h"

using namespace coev;

awaiter echo(iocontext &c, Httpparser &req)
{
	LOG_DBG("recv echo\n");
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
	req.m_response = oss.str();
	co_await c.send(req.m_response.data(), req.m_response.size());
	co_return 0;
}

int main()
{
	set_log_level(LOG_LEVEL_DEBUG);
	Httpserver pool("0.0.0.0", 9999, 8);

	pool.add_router("/echo", echo);
	running::instance().join();
	return 0;
}
