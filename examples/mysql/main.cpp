/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coapp.h>
#include <string.h>
#include <sstream>
#include <random>

using namespace coev;

struct t_test_table
{
	int id;
	std::string username;
	std::string password;
	std::string mobile;
	std::string create_time;
	std::string update_time;
};
Awaiter<int> go()
{
	Mysqlcli c("127.0.01", 3306, "ashan", "12345678", "test");
	auto r = co_await c.connect();

	std::ostringstream oss;
	std::default_random_engine e;
	oss << "INSERT INTO t_test_table "
		<< "(`f_username`,`f_password`,`f_mobile`,`f_state`,`f_content`) "
		<< "VALUES ";
	auto f = [&]()
	{
		oss << "('" << e() << e() << "','" << e() << e() << "','"
			<< e() << e() << "','" << e() << e() << "','" << e() << e() << "')";
	};
	f();
	for (int i = 0; i < 50000; i++)
	{
		oss << ",";
		f();
	}
	oss << ";";
	auto s = oss.str();
	LOG_DBG("%s\n", s.c_str());
	r = co_await c.query(s.c_str(), s.size(), [](auto, auto) {});
	if (r == INVALID)
	{
		throw("error");
	}
	t_test_table t;
	for (int i = 0; i < 3; i++)
	{
		oss.str("");
		oss << "SELECT * FROM t_test_table;";
		auto s = oss.str();
		r = co_await c.query(
			s.c_str(), s.size(),
			[&](auto, auto rows)
			{
				unpack(rows, t.id, t.username, t.password, t.mobile, t.create_time, t.update_time);
				LOG_DBG("%d %s %s %s %s %s\n", t.id, t.username.data(), t.password.data(),
						t.mobile.data(), t.create_time.data(), t.update_time.data());
			});
		if (r == INVALID)
		{
			LOG_ERR("error %d\n", r);
			throw("error");
			co_return 0;
		}
	}

	LOG_DBG("truncate table begin\n");
	oss.str("");
	oss << "truncate table t_test_table;";
	s = oss.str();
	r = co_await c.query(s.c_str(), s.size(), [](auto, auto) {});

	if (r == INVALID)
	{
		throw("error");
	}

	LOG_FATAL("SUCCESS\n");
	co_return 0;
}
Awaiter<int> clear()
{
	Mysqlcli c("127.0.01", 3306, "ashan", "12345678", "test");
	auto r = co_await c.connect();
	if (r == INVALID)
	{
		co_return INVALID;
	}
	std::ostringstream oss;
	oss << "truncate table t_test_table;";
	auto s = oss.str();
	r = co_await c.query(s.c_str(), s.size(), [](auto, auto) {});
	if (r == INVALID)
	{
		throw("error");
	}
	co_return 0;
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);
	Routine r;
	r.add(go);
	// r.add(clear);
	r.join();
	return 0;
}