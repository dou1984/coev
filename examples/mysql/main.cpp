/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <string.h>
#include <sstream>
#include <random>
#include <coev/coev.h>
#include <coev_mysql/Mysql.h>

using namespace coev;
using namespace coev::pool;
struct t_test_table
{
	int id;
	std::string username;
	std::string password;
	std::string mobile;
	std::string create_time;
	std::string update_time;
};

Mysql mysql;

awaitable<void> create()
{
	auto sql = R"(CREATE TABLE IF NOT EXISTS t_test_table (
    id INT AUTO_INCREMENT PRIMARY KEY COMMENT '主键ID',
    f_username VARCHAR(50) NOT NULL COMMENT '用户名',
    f_password VARCHAR(255) NOT NULL COMMENT '密码(加密存储)',
    f_mobile VARCHAR(20) DEFAULT NULL COMMENT '手机号',
    f_state VARCHAR(20) DEFAULT NULL COMMENT '用户状态：1-正常；2-禁用；3-删除',
    f_content VARCHAR(255) DEFAULT NULL COMMENT '用户简介',
    create_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    UNIQUE KEY uk_username (f_username),
    INDEX idx_mobile (f_mobile)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '用户测试表';)";

	Mysql::instance c;
	auto err = co_await mysql.get(c);
	if (err == INVALID)
	{
		LOG_ERR("connect error ");
		co_return;
	}
	co_await c->query(sql);

	err = c->results();
	LOG_DBG("results %d", err);
}
awaitable<void> go()
{
	Mysql::instance c;
	auto err = co_await mysql.get(c);
	if (err == INVALID)
	{
		LOG_ERR("connect error ");
		co_return;
	}
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
	LOG_DBG("%s", s.c_str());
	err = co_await c->query(s);
	if (err == INVALID)
	{
		LOG_ERR("insert error, but continuing");
		// Don't throw, just continue
	}
	else
	{
		c->results();
	}
	t_test_table t;

	oss.str("");
	oss << "SELECT * FROM t_test_table;";
	s = oss.str();
	err = co_await c->query(s);
	if (err == INVALID)
	{
		LOG_ERR("%d", err);
		co_return;
	}
	LOG_DBG("query size:%d", err);

	while (true)
	{
		err = c->results(t.id, t.username, t.password, t.mobile, t.create_time, t.update_time);
		if (err != 0)
		{
			break;
		}
		LOG_DBG("%d %s %s %s %s %s", t.id, t.username.data(), t.password.data(),
			t.mobile.data(), t.create_time.data(), t.update_time.data());
	}
	if (err == INVALID)
	{
		LOG_ERR("error %d", err);
		co_return;
	}
	LOG_INFO("SUCCESS");
}
awaitable<void> clear()
{
	Mysql::instance c;
	auto err = co_await mysql.get(c);
	if (err == INVALID)
	{
		co_return;
	}
	std::ostringstream oss;
	oss << "truncate table t_test_table;";
	auto s = oss.str();
	err = co_await c->query(s);
	if (err == INVALID)
	{
		throw("error");
	}
	err = c->results();
	if (err == 0)
	{
		LOG_DBG("results %d", err);
	}
	co_return;
}
int main()
{
	set_log_level(LOG_LEVEL_DEBUG);

	auto conf = mysql.get_config();
	conf->host = "127.0.0.1";
	conf->port = 3306;
	conf->username = "root";
	conf->password = "abcd@12345678";
	conf->options["charset"] = "utf8mb4";
	conf->options["db"] = "test";

	mysql.set(conf);
	coev::runnable::instance()
		.start(
			[]() -> awaitable<void>
			{
				co_await create();

				co_await go();

				co_await clear();
			})
		.wait();

	return 0;
}