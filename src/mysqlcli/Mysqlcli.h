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
#include "../coev.h"

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
	class Mysqlcli : Mysqlconf, public EVRecv, public EVSend
	{
	public:
		Mysqlcli(const char *ip, int port, const char *username, const char *password, const char *db);
		virtual ~Mysqlcli();

		Awaiter<int> connect();
		Awaiter<int> query(const char *sql, int size, const std::function<void(int, MYSQL_ROW)> &);
		Awaiter<int> query(const char *sql, int size);

	private:
		MYSQL *m_mysql = nullptr;
		int m_tag = 0;
		ev_io m_Read;
		ev_io m_Write;

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