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
		_this->m_write_waiter.resume();
	}
	void Mysqlcli::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		Mysqlcli *_this = (Mysqlcli *)(w->data);
		assert(_this != nullptr);
		_this->m_read_waiter.resume();
	}
	Mysqlcli::Mysqlcli(const Mysqlconf &conf)
	{
		m_mysql = mysql_init(0);
		if (m_mysql == nullptr)
		{
			throw("mysql init");
		}
		m_url = conf.m_url;
		m_port = conf.m_port;
		m_username = conf.m_username;
		m_password = conf.m_password;
		m_db = conf.m_db;
		m_charset = conf.m_charset;
	}
	Mysqlcli::~Mysqlcli()
	{
		mysql_close(m_mysql);
	}
	int Mysqlcli::__tryconnect()
	{
		return mysql_real_connect_nonblocking(
			m_mysql, m_url.c_str(), m_username.c_str(), m_password.c_str(), m_db.c_str(), m_port, nullptr, 0);
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
		m_loop = cosys::data();
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
		m_read.data = this;
		ev_io_init(&m_read, &Mysqlcli::cb_read, fd(), EV_READ | EV_WRITE);
		ev_io_start(m_loop, &m_read);
		return 0;
	}
	int Mysqlcli::__connect_remove()
	{
		if (fd() != INVALID)
		{
			ev_io_stop(m_loop, &m_read);
		}
		return 0;
	}
	int Mysqlcli::__query_insert()
	{
		assert(fd() != INVALID);
		TRACE();
		m_read.data = this;
		ev_io_init(&m_read, &Mysqlcli::cb_read, fd(), EV_READ);
		ev_io_start(m_loop, &m_read);
		m_write.data = this;
		ev_io_init(&m_write, &Mysqlcli::cb_write, fd(), EV_WRITE);
		return 0;
	}
	int Mysqlcli::__query_remove()
	{
		if (fd() != INVALID)
		{
			ev_io_stop(m_loop, &m_read);
			ev_io_stop(m_loop, &m_write);
		}
		return 0;
	}
	int Mysqlcli::fd()
	{
		return mysql_get_socket_descriptor(m_mysql);
	}
	awaitable<int> Mysqlcli::connect()
	{
		if (__connect() == INVALID)
			co_return INVALID;
		co_await m_read_waiter.suspend();
		__connect_remove();
		__query_insert();
		co_await m_read_waiter.suspend();
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
	awaitable<int> Mysqlcli::query(const char *sql, int size)
	{
		__clear();
		int status = 0;
		while ((status = mysql_send_query_nonblocking(m_mysql, sql, size)) == NET_ASYNC_NOT_READY)
		{
			if (__isneterror(status) == INVALID)
			{
				co_return INVALID;
			}
			if (isInprocess())
			{
				ev_io_start(m_loop, &m_write);
				co_await m_write_waiter.suspend();
				ev_io_stop(m_loop, &m_write);
			}
		}
		if (__isneterror(status) == INVALID)
		{
			LOG_CORE("error %d %d %s\n", status, mysql_errno(m_mysql), mysql_error(m_mysql));
			co_return INVALID;
		}
		do
		{
			co_await m_read_waiter.suspend();
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

		assert(m_results == nullptr);
		while ((status = mysql_store_result_nonblocking(m_mysql, &m_results)) == NET_ASYNC_NOT_READY)
		{
		}
		auto num_rows = mysql_affected_rows(m_mysql);
		co_return num_rows;
	}
	int Mysqlcli::__results()
	{
		if (m_results == nullptr)
		{
			auto last_error = mysql_errno(m_mysql);
			LOG_CORE("last error %d\n", last_error);
			return last_error;
		}
		int status = 0;
		if ((status = mysql_fetch_row_nonblocking(m_results, &m_row)) == NET_ASYNC_COMPLETE && m_row)
		{
			assert(mysql_errno(m_mysql) == 0);
			return status;
		}
		return NET_ASYNC_COMPLETE_NO_MORE_RESULTS;
	}
	void Mysqlcli::__clear()
	{
		int status = 0;
		while ((status = mysql_free_result_nonblocking(m_results)) != NET_ASYNC_COMPLETE)
		{
		}
		m_row = nullptr;
		m_results = nullptr;
	}
}