/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "Mysqlcli.h"

namespace coev
{
	bool is_network_error(uint errorno)
	{
		return errorno == CR_CONNECTION_ERROR || errorno == CR_CONN_HOST_ERROR ||
			   errorno == CR_SERVER_GONE_ERROR || errorno == CR_SERVER_LOST ||
			   errorno == ER_CON_COUNT_ERROR || errorno == ER_SERVER_SHUTDOWN ||
			   errorno == ER_NET_READ_INTERRUPTED ||
			   errorno == ER_NET_WRITE_INTERRUPTED;
	}
	int mysql_get_socket_descriptor(MYSQL *mysql)
	{
		if (mysql && mysql->net.vio)
		{
			return *(int *)(mysql->net.vio);
		}
		return INVALID;
	}
	void Mysqlcli::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		Mysqlcli *_this = (Mysqlcli *)(w->data);
		assert(_this != nullptr);
		resume<1>(_this);
	}
	void Mysqlcli::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		Mysqlcli *_this = (Mysqlcli *)(w->data);
		assert(_this != nullptr);
		resume<0>(_this);
	}
	Mysqlcli::Mysqlcli(const char *ip, int port, const char *username, const char *password, const char *db)
	{
		m_mysql = mysql_init(0);
		if (m_mysql == nullptr)
		{
			throw("mysql init");
		}
		m_ip = ip;
		m_port = port;
		m_username = username;
		m_password = password;
		m_db = db;
	}
	Mysqlcli::~Mysqlcli()
	{
		mysql_close(m_mysql);
	}
	int Mysqlcli::__tryconnect()
	{
		return mysql_real_connect_nonblocking(
			m_mysql, m_ip.c_str(), m_username.c_str(), m_password.c_str(), m_db.c_str(), m_port, nullptr, 0);
	}
	int Mysqlcli::__isneterror(int state)
	{
		if (state != NET_ASYNC_ERROR)
			return 0;
		if (!is_network_error(mysql_errno(m_mysql)))
			return 0;
		__query_remove();
		return INVALID;
	}
	int Mysqlcli::__isqueryerror(int state)
	{
		return state != NET_ASYNC_ERROR ? 0 : INVALID;
	}
	int Mysqlcli::__connect()
	{
		m_tid = gtid();
		auto status = __tryconnect();
		if (status == NET_ASYNC_ERROR)
		{
			LOG_CORE("mysql connect %d error:%d %s\n", status, mysql_errno(m_mysql), mysql_error(m_mysql));
			return INVALID;
		}
		assert(status == NET_ASYNC_NOT_READY);
		LOG_CORE("status:%d fd:%d\n", status, fd());
		__connect_insert();
		return 0;
	}
	int Mysqlcli::__connect_insert()
	{
		assert(fd() != INVALID);
		TRACE();
		m_Read.data = this;
		ev_io_init(&m_Read, &Mysqlcli::cb_read, fd(), EV_READ | EV_WRITE);
		ev_io_start(loop::at(m_tid), &m_Read);
		return 0;
	}
	int Mysqlcli::__connect_remove()
	{
		if (fd() != INVALID)
		{
			ev_io_stop(loop::at(m_tid), &m_Read);
		}
		return 0;
	}
	int Mysqlcli::__query_insert()
	{
		assert(fd() != INVALID);
		TRACE();
		m_Read.data = this;
		ev_io_init(&m_Read, &Mysqlcli::cb_read, fd(), EV_READ);
		ev_io_start(loop::at(m_tid), &m_Read);
		m_Write.data = this;
		ev_io_init(&m_Write, &Mysqlcli::cb_write, fd(), EV_WRITE);
		return 0;
	}
	int Mysqlcli::__query_remove()
	{
		if (fd() != INVALID)
		{
			ev_io_stop(loop::at(m_tid), &m_Read);
			ev_io_stop(loop::at(m_tid), &m_Write);
		}
		return 0;
	}
	int Mysqlcli::fd()
	{
		return mysql_get_socket_descriptor(m_mysql);
	}
	awaiter Mysqlcli::connect()
	{
		if (__connect() == INVALID)
			co_return INVALID;
		co_await wait_for<0>(this);
		__connect_remove();
		__query_insert();
		co_await wait_for<0>(this);
		int status = 0;
		while ((status = __tryconnect()) == NET_ASYNC_NOT_READY)
		{
		}
		if (__isneterror(NET_ASYNC_ERROR) == INVALID)
		{
			co_return INVALID;
		}
		co_return fd();
	}
	awaiter Mysqlcli::query(const char *sql, int size)
	{
		int status = 0;
		while ((status = mysql_send_query_nonblocking(m_mysql, sql, size)) == NET_ASYNC_NOT_READY)
		{
			if (__isneterror(status) == INVALID)
			{
				co_return INVALID;
			}
			if (isInprocess())
			{
				ev_io_start(loop::at(m_tid), &m_Write);
				co_await wait_for<1>(this);
				ev_io_stop(loop::at(m_tid), &m_Write);
			}
		}
		if (__isneterror(status) == INVALID)
		{
			LOG_CORE("error %d %d %s\n", status, mysql_errno(m_mysql), mysql_error(m_mysql));
			co_return INVALID;
		}
		do
		{
			co_await wait_for<0>(this);
		} while ((status = mysql_real_query_nonblocking(m_mysql, sql, size)) == NET_ASYNC_NOT_READY);
		if (__isneterror(status) == INVALID)
		{
			LOG_CORE("error %d %d %s\n", status, mysql_errno(m_mysql), mysql_error(m_mysql));
			co_return INVALID;
		}
		if (__isqueryerror(status) == INVALID)
		{
			LOG_CORE("error %d %d %s\n", status, mysql_errno(m_mysql), mysql_error(m_mysql));
			co_return INVALID;
		}
		co_return 0;
	}
	awaiter Mysqlcli::query(const char *sql, int size, const std::function<void(int, MYSQL_ROW)> &callback)
	{
		auto r = co_await query(sql, size);
		if (r == INVALID)
		{
			co_return r;
		}
		int status = 0;
		MYSQL_RES *results = nullptr;
		while ((status = mysql_store_result_nonblocking(m_mysql, &results)) == NET_ASYNC_NOT_READY)
		{
		}
		int num_rows = mysql_affected_rows(m_mysql);
		if (results != nullptr)
		{
			int i = 0;
			MYSQL_ROW rows;
			while ((status = mysql_fetch_row_nonblocking(results, &rows)) == NET_ASYNC_COMPLETE && rows)
			{
				assert(mysql_errno(m_mysql) == 0);
				callback(i++, rows);
			}
			while ((status = mysql_free_result_nonblocking(results)) != NET_ASYNC_COMPLETE)
			{
			}

			co_return num_rows;
		}
		else
		{
			auto last_error = mysql_errno(m_mysql);
			LOG_CORE("last error %d\n", last_error);
			co_return last_error == 0 ? num_rows : INVALID;
		}
		co_return 0;
	}
}