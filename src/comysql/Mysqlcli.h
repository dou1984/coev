/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <tuple>
#include <ev.h>
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>
#include "../cosys/cosys.h"

namespace coev
{
	struct Mysqlconf
	{
		std::string m_url;
		std::string m_username;
		std::string m_password;
		std::string m_db;
		std::string m_charset;
		int m_port;
	};
	class Mysqlcli : Mysqlconf
	{
	public:
		Mysqlcli(const Mysqlconf &);
		virtual ~Mysqlcli();
		operator MYSQL *() { return m_mysql; }

		awaitable<int> connect();
		awaitable<int> query(const char *sql, int size);
		awaitable<int> query(const std::string &sql)
		{
			return query(sql.c_str(), sql.size());
		}

		template <class... TYPE>
		constexpr int results(TYPE &...value)
		{
			auto last_error = __results();
			if (last_error == 0)
			{
				int i = 0;
				(__setvalue(value, m_row[i++]), ...);
			}
			return last_error;
		}

	private:
		MYSQL *m_mysql = nullptr;
		MYSQL_RES *m_results = nullptr;
		MYSQL_ROW m_row = nullptr;
		int m_tid = 0;
		struct ev_loop *m_loop = nullptr;
		ev_io m_read;
		ev_io m_write;
		async m_read_waiter;
		async m_write_waiter;

		int __results();
		void __clear();

		static void cb_connect(struct ev_loop *loop, struct ev_io *w, int revents);
		static void cb_write(struct ev_loop *loop, struct ev_io *w, int revents);
		static void cb_read(struct ev_loop *loop, struct ev_io *w, int revents);
		int __tryconnect();
		int __isneterror(int);
		int __isqueryerror(int state);
		int __connect();
		int __connect_insert();
		int __connect_remove();
		int __query_insert();
		int __query_remove();
		int fd();
	};
}