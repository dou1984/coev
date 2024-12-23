/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev/coev.h>
#include <sstream>
#include <random>
#include <string_view>
#include <vector>
#include <cosys/cosys.h>
#include <Rediscli/rediscli.h>

using namespace coev;

awaitable<int> go()
{
	set_log_level(LOG_LEVEL_DEBUG);

	Rediscli c("127.0.0.1", 6379, "");

	co_await c.connect();

	co_await c.query("ping hello");

	if (c.error())
	{
		co_return 0;
	}
	std::default_random_engine e;
	std::ostringstream oss;
	oss << "HSET test_0000 ";
	auto f = [&]()
	{
		oss << e() << " " << e() << " ";
	};
	for (int i = 0; i < 100; i++)
	{
		f();
	}
	auto s = oss.str();
	co_await c.query(s);
	if (c.error())
	{
		co_return 0;
	}
	s = c.reply();
	std::cout << s << std::endl;

	oss.str("");
	oss << "del test_0000 ";
	s = oss.str();
	co_await c.query(s);

	if (c.error())
	{
		co_return c.reply_integer();
	}
	s = c.reply();
	co_return 0;
}

awaitable<int> test_sync()
{
	Rediscli c("127.0.0.1", 6379, "");
	co_await c.connect();

	co_await c.query("info server");

	auto s = c.reply_string();
	LOG_DBG("%s\n", s.c_str());

	co_return 0;
}

int main()
{
	running::instance()
		// .add(test_sync)
		.add(go)
		.join();
	return 0;
}