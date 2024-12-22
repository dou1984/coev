/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <sstream>
#include <cosys/cosys.h>
#include <httpparser/Httpparser.h>
#include <httpparser/Httpserver.h>

using namespace coev;

awaitable<int> echo(iocontext &io)
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
	co_await io.send(response.data(), response.size());
	co_return 0;
}

awaitable<void> co_router(Httpserver &pool)
{
	while (pool.get())
	{
		host h;
		auto fd = co_await pool.accept(h);

		[=]() -> awaitable<void>
		{
			iocontext io(fd);
			Httpparser req;
			co_await wait_for_all(
				Httpserver::parse(io, req),
				[&]() -> awaitable<void>
				{
					co_await req.get_url();

					while (true)
					{
						auto [key, value] = co_await req.get_header();
						if (key == "")
						{
							break;
						}
						std::cout << key << "=" << value << std::endl;
					}

					auto body = co_await req.get_body();

					std::cout << body << std::endl;

					co_await echo(io);

					co_await io.close();
				}());
		}();
	}
}
int main()
{
	set_log_level(LOG_LEVEL_DEBUG);
	Httpserver pool("0.0.0.0", 9999);

	running::instance()
		.add(
			8,
			[&]() -> awaitable<void>
			{
				co_await co_router(pool);
			})
		.join();
	return 0;
}
