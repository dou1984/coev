/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <string.h>
#include <sstream>
#include <random>
#include <coev/coev.h>
#include <coev_mysql/MysqlCli.h>

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
awaitable<void> go()
{
	LOG_ERR("begin\n");
	coev::MysqlCli c({
		.m_url = "127.0.0.1",
		.m_username = "ashan",
		.m_password = "12345678",
		.m_db = "test",
		.m_charset = "",
		.m_port = 3306,
	});
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
	for (int i = 0; i < 50; i++)
	{
		oss << ",";
		f();
	}
	oss << ";";
	auto s = oss.str();
	LOG_DBG("%s\n", s.c_str());
	r = co_await c.query(s);
	if (r == INVALID)
	{
		throw("error");
	}
	c.results();
	t_test_table t;

	oss.str("");
	oss << "SELECT * FROM t_test_table;";
	s = oss.str();
	r = co_await c.query(s);
	if (r == INVALID)
	{
		LOG_ERR("%d\n", r);
	}
	LOG_DBG("query size:%d\n", r);

	while (true)
	{
		r = c.results(t.id, t.username, t.password, t.mobile, t.create_time, t.update_time);
		if (r != 0)
		{
			break;
		}
		LOG_DBG("%d %s %s %s %s %s\n", t.id, t.username.data(), t.password.data(),
				t.mobile.data(), t.create_time.data(), t.update_time.data());
	}
	if (r == INVALID)
	{
		LOG_ERR("error %d\n", r);
		throw("error");
	}

	LOG_DBG("truncate table begin\n");
	oss.str("");
	oss << "truncate table t_test_table;";
	s = oss.str();
	r = co_await c.query(s);
	if (r == INVALID)
	{
		throw("error");
	}
	c.results();
	LOG_INFO("SUCCESS\n");
}
awaitable<void> clear()
{
	coev::MysqlCli c({
		.m_url = "127.0.0.1",
		.m_username = "ashan",
		.m_password = "12345678",
		.m_db = "test",
		.m_charset = "",
		.m_port = 3306,
	});
	auto r = co_await c.connect();
	if (r == INVALID)
	{
		co_return;
	}
	std::ostringstream oss;
	oss << "truncate table t_test_table;";
	auto s = oss.str();
	r = co_await c.query(s);
	if (r == INVALID)
	{
		throw("error");
	}
	auto err = c.results();
	if (err == 0)
	{
		LOG_DBG("results %d\n", err);
	}
	co_return;
}
int main()
{
	set_log_level(LOG_LEVEL_DEBUG);

	coev::runnable::instance()
		.start(go)
		.join();

	return 0;
}