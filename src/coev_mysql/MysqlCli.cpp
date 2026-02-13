/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "MysqlCli.h"

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
	void MysqlCli::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		MysqlCli *_this = (MysqlCli *)(w->data);
		assert(_this != nullptr);
		_this->m_w_waiter.resume();
		local_resume();
	}
	void MysqlCli::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		MysqlCli *_this = (MysqlCli *)(w->data);
		assert(_this != nullptr);
		_this->m_r_waiter.resume();
		local_resume();
	}
	MysqlCli::MysqlCli(const MysqlConf &conf)
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
	MysqlCli::~MysqlCli()
	{
		mysql_close(m_mysql);
	}
	int MysqlCli::__try_connect()
	{
		return mysql_real_connect_nonblocking(
			m_mysql, m_url.c_str(), m_username.c_str(), m_password.c_str(), m_db.c_str(), m_port, nullptr, 0);
	}
	int MysqlCli::__is_net_error(int state)
	{
		if (state != NET_ASYNC_ERROR)
			return 0;
		if (!is_network_error(mysql_errno(m_mysql)))
			return 0;
		__query_remove();
		return INVALID;
	}
	int MysqlCli::__is_query_error(int state)
	{
		return state != NET_ASYNC_ERROR ? 0 : INVALID;
	}
	int MysqlCli::__connect()
	{
		m_tid = gtid();
		m_loop = cosys::data();
		auto status = __try_connect();
		if (status == NET_ASYNC_ERROR)
		{
			LOG_CORE("mysql connect %d error:%d %s", status, mysql_errno(m_mysql), mysql_error(m_mysql));
			return INVALID;
		}
		assert(status == NET_ASYNC_NOT_READY);
		LOG_CORE("status:%d fd:%d", status, fd());
		__connect_insert();
		return 0;
	}
	int MysqlCli::__connect_insert()
	{
		assert(fd() != INVALID);
		m_read.data = this;
		ev_io_init(&m_read, &MysqlCli::cb_read, fd(), EV_READ | EV_WRITE);
		ev_io_start(m_loop, &m_read);
		return 0;
	}
	int MysqlCli::__connect_remove()
	{
		if (fd() != INVALID)
		{
			ev_io_stop(m_loop, &m_read);
		}
		return 0;
	}
	int MysqlCli::__query_insert()
	{
		assert(fd() != INVALID);
		m_read.data = this;
		ev_io_init(&m_read, &MysqlCli::cb_read, fd(), EV_READ);
		ev_io_start(m_loop, &m_read);
		m_write.data = this;
		ev_io_init(&m_write, &MysqlCli::cb_write, fd(), EV_WRITE);
		return 0;
	}
	int MysqlCli::__query_remove()
	{
		if (fd() != INVALID)
		{
			ev_io_stop(m_loop, &m_read);
			ev_io_stop(m_loop, &m_write);
		}
		return 0;
	}
	int MysqlCli::fd()
	{
		return mysql_get_socket_descriptor(m_mysql);
	}
	awaitable<int> MysqlCli::connect()
	{
		if (__connect() == INVALID)
			co_return INVALID;
		co_await m_r_waiter.suspend();
		__connect_remove();
		__query_insert();
		co_await m_r_waiter.suspend();
		int status = 0;
		while ((status = __try_connect()) == NET_ASYNC_NOT_READY)
		{
		}
		if (__is_net_error(NET_ASYNC_ERROR) == INVALID)
		{
			co_return INVALID;
		}
		co_return fd();
	}
	awaitable<int> MysqlCli::query(const char *sql, int size)
	{
		__clear();
		int status = 0;
		while ((status = mysql_send_query_nonblocking(m_mysql, sql, size)) == NET_ASYNC_NOT_READY)
		{
			if (__is_net_error(status) == INVALID)
			{
				co_return INVALID;
			}
			if (isInprocess())
			{
				ev_io_start(m_loop, &m_write);
				co_await m_w_waiter.suspend();
				ev_io_stop(m_loop, &m_write);
			}
		}
		if (__is_net_error(status) == INVALID)
		{
			LOG_CORE("error %d %d %s", status, mysql_errno(m_mysql), mysql_error(m_mysql));
			co_return INVALID;
		}
		do
		{
			co_await m_r_waiter.suspend();
		} while ((status = mysql_real_query_nonblocking(m_mysql, sql, size)) == NET_ASYNC_NOT_READY);
		if (__is_net_error(status) == INVALID)
		{
			LOG_CORE("error %d %d %s", status, mysql_errno(m_mysql), mysql_error(m_mysql));
			co_return INVALID;
		}
		if (__is_query_error(status) == INVALID)
		{
			LOG_CORE("error %d %d %s", status, mysql_errno(m_mysql), mysql_error(m_mysql));
			co_return INVALID;
		}

		assert(m_results == nullptr);
		while ((status = mysql_store_result_nonblocking(m_mysql, &m_results)) == NET_ASYNC_NOT_READY)
		{
		}
		auto num_rows = mysql_affected_rows(m_mysql);
		co_return num_rows;
	}
	int MysqlCli::__results()
	{
		if (m_results == nullptr)
		{
			auto last_error = mysql_errno(m_mysql);
			LOG_CORE("last error %d", last_error);
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
	void MysqlCli::__clear()
	{
		int status = 0;
		while ((status = mysql_free_result_nonblocking(m_results)) != NET_ASYNC_COMPLETE)
		{
		}
		m_row = nullptr;
		m_results = nullptr;
	}
}