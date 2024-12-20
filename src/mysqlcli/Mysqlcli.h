/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>
#include <ev.h>
#include "../cosys/cosys.h"

namespace coev
{
	struct Mysqlconf
	{
		std::string m_ip;
		std::string m_username;
		std::string m_password;
		std::string m_db;
		int m_port;
	};
	class Mysqlcli : Mysqlconf
	{
	public:
		Mysqlcli(const char *ip, int port, const char *username, const char *password, const char *db);
		virtual ~Mysqlcli();
		operator MYSQL *() { return m_mysql; }

		awaitable<int> connect();
		awaitable<int> query(const char *sql, int size, const std::function<void(int, MYSQL_ROW)> &);
		awaitable<int> query(const char *sql, int size);

		template <class... TYPE>
		void result(MYSQL_ROW rows, TYPE &...value)
		{
			int i = 0;
			(coev::__setvalue(value, rows[i++]), ...);
		}
	private:
		MYSQL *m_mysql = nullptr;
		int m_tid = 0;
		ev_io m_read;
		ev_io m_write;
		async m_read_listener;
		async m_write_listener;

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