/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev.h>
#include <coapp.h>
#include <sstream>
#include <random>

using namespace coev;

awaiter go()
{
	set_log_level(LOG_LEVEL_CORE);

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

int main()
{
	routine::instance().add(go);
	routine::instance().join();
	return 0;
}