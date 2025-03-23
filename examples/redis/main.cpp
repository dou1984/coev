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
#include <coev/coev.h>
#include <co_redis/Rediscli.h>

using namespace coev;

awaitable<void> go()
{
	set_log_level(LOG_LEVEL_DEBUG);

	Rediscli c("127.0.0.1", 6379, "");

	co_await c.connect();

	co_await c.query("ping hello");

	if (c.error())
	{
		co_return;
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
		co_return;
	}
	s = c.reply();
	std::cout << s << std::endl;

	oss.str("");
	oss << "del test_0000 ";
	s = oss.str();
	co_await c.query(s);

	if (c.error())
	{
		c.reply_integer();
		co_return;
	}
	s = c.reply();
	co_return;
}

awaitable<void> test_sync()
{
	Rediscli c("127.0.0.1", 6379, "");
	co_await c.connect();

	co_await c.query("info server");

	auto s = c.reply();
	LOG_DBG("%s\n", s.c_str());
}
awaitable<void> test_array()
{
	Rediscli c("127.0.0.1", 6379, "");
	co_await c.connect();

	std::ostringstream oss;
	oss << "hset h:test" << " a 1 " << " b 2 " << " c 3 ";

	auto s = oss.str();
	co_await c.query(s);
	if (c.error())
	{
		LOG_ERR("test_array %s\n", c.reply().c_str());
		co_return;
	}
	LOG_DBG("hset h:test %s\n", c.reply().c_str());

	oss.str("");
	oss << "hgetall h:test";
	s = oss.str();
	co_await c.query(s);

	if (c.error())
	{
		LOG_ERR("test_array hgeall %s\n", c.reply().c_str());
		co_return;
	}
	auto arr = c.reply_array();
	std::string key;
	std::string value;

	while (arr)
	{
		arr = arr.reply(key, value);
		LOG_DBG("%s=%s\n", key.c_str(), value.c_str());
	}

	oss.str("");
	oss << "del h:test";
	co_await c.query(oss.str());
	if (c.error())
	{
		LOG_DBG("del h:test error %s\n", c.reply().c_str());
		co_return;
	}
	s = c.reply();

	LOG_DBG("del h:test %s\n", s.c_str());
	co_return;
}
int main()
{
	runnable::instance()
		.add(test_sync)
		.add(go)
		.add(test_array)
		.join();
	return 0;
}