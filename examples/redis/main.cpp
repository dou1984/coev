/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev.h>
#include <sstream>
#include <random>
#include <string_view>
#include <vector>
#include "../cosys/coapp.h"

using namespace coev;

awaiter go()
{
	set_log_level(LOG_LEVEL_DEBUG);

	Rediscli c("127.0.0.1", 6379, "");

	co_await c.connect();

	co_await c.query("ping hello", [](auto &r)
					 { LOG_DBG("%s\n", r.last_msg); });

	if (!c)
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
	co_await c.query(
		s.data(),
		[=](auto &r)
		{
			if (r.last_error == INVALID)
			{
				LOG_DBG("%d %s\n", r.last_error, r.last_msg);
			}
			else
			{
				LOG_DBG("%d\n", r.last_error);
			}
		});
	if (!c)
	{
		co_return 0;
	}
	oss.str("");
	oss << "del test_0000 ";
	s = oss.str();
	co_await c.query(s.data(), [](auto &r) {});
	co_return 0;
}

auto __trim(const char *str, char c)
{
	std::vector<std::string> r;
	std::string_view begin(str);
	while (true)
	{
		if (auto p0 = begin.find_first_not_of(c); p0 == std::string_view::npos)
		{
			return r;
		}
		else if (auto p1 = begin.find_first_of(c, p0); p1 == std::string_view::npos)
		{
			r.emplace_back(begin.data() + p0);
			return r;
		}
		else
		{

			r.emplace_back(begin.data() + p0, begin.data() + p1);
			begin = std::string_view(begin.data() + p1);
		}
	}
}
awaiter test_sync()
{
	Rediscli c("127.0.0.1", 6379, "");
	co_await c.connect();

	co_await c.query("info server");

	LOG_DBG("%s\n", c.result().str);

	co_await c.query("PSYNC ? -1");
	LOG_DBG("%s\n", c.result().str);

	co_await c.query("SUBSCRIBE");
	LOG_DBG("%s\n", c.result().str);

	while (c)
	{
		co_await wait_for(c);
		LOG_DBG("%d\n%s\n", c.result().num_rows, c.result().str);
	}
	co_return 0;
}
awaiter test_subscirbe()
{

	Rediscli c("127.0.0.1", 6379, "");
	co_await c.connect();

	co_await c.query("PSUBSCRIBE KEY:*");
	std::string out[3];
	c.result().unpack(out[0], out[1], out[2]);
	LOG_DBG("callback recv begin %s %s %s\n", out[0].data(), out[1].data(), out[2].data());

	while (c)
	{
		co_await wait_for(c);
		std::string out[4];
		c.result().unpack(out[0], out[1], out[2], out[3]);
		LOG_DBG("recv %s %s %s %s\n", out[0].data(), out[1].data(), out[2].data(), out[3].data());
	}

	co_return 0;
}
int main()
{
	running::instance().add(test_sync).join();
	return 0;
}