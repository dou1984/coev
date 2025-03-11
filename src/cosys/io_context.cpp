/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "cosys.h"
#include "io_context.h"

namespace coev
{
	void io_context::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (io_context *)w->data;
		assert(_this != NULL);
		_this->m_read_waiter.resume();
	}
	void io_context::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (io_context *)w->data;
		assert(_this != NULL);
		_this->m_write_waiter.resume();
		_this->__del_write();
	}
	int io_context::__close()
	{
		if (m_fd != INVALID)
		{
			__finally();
			::close(m_fd);
			m_fd = INVALID;
			while (m_read_waiter.resume())
			{
			}
			while (m_write_waiter.resume())
			{
			}
		}
		return 0;
	}
	bool io_context::__valid() const
	{
		return m_fd != INVALID;
	}
	io_context::operator bool() const
	{
		return m_fd != INVALID;
	}
	io_context::io_context(int fd) : m_fd(fd)
	{
		m_tid = gtid();
		m_loop = cosys::data();
		__initial();
	}
	int io_context::__initial()
	{
		LOG_CORE("fd:%d\n", m_fd);
		if (m_fd != INVALID)
		{
			setNoBlock(m_fd, true);

			m_read.data = this;
			ev_io_init(&m_read, io_context::cb_read, m_fd, EV_READ);
			ev_io_start(m_loop, &m_read);

			m_write.data = this;
			ev_io_init(&m_write, io_context::cb_write, m_fd, EV_WRITE);
		}
		return 0;
	}
	int io_context::__finally()
	{
		LOG_CORE("fd:%d\n", m_fd);
		if (m_fd != INVALID)
		{
			ev_io_stop(m_loop, &m_read);
			ev_io_stop(m_loop, &m_write);
		}
		return 0;
	}
	io_context::~io_context()
	{
		__close();
	}
	awaitable<int> io_context::send(const char *buffer, int size)
	{
		while (__valid())
		{
			auto r = ::send(m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				ev_io_start(m_loop, &m_write);
				co_await m_write_waiter.suspend();
			}
			else
			{
				co_return r;
			}
		}
		co_return INVALID;
	}
	awaitable<int> io_context::recv(char *buffer, int size)
	{
		while (__valid())
		{
			co_await m_read_waiter.suspend();
			auto r = ::recv(m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				continue;
			}
			co_return r;
		}
		co_return INVALID;
	}
	awaitable<int> io_context::recvfrom(char *buffer, int size, addrInfo &info)
	{
		while (__valid())
		{
			co_await m_read_waiter.suspend();
			sockaddr_in addr;
			socklen_t addrsize = sizeof(addr);
			int r = ::recvfrom(m_fd, buffer, size, 0, (struct sockaddr *)&addr, &addrsize);
			if (r == INVALID && isInprocess())
			{
				continue;
			}
			parseAddr(addr, info);
			co_return r;
		}
		co_return INVALID;
	}
	awaitable<int> io_context::sendto(const char *buffer, int size, addrInfo &info)
	{
		while (__valid())
		{
			sockaddr_in addr;
			fillAddr(addr, info.ip, info.port);
			int r = ::sendto(m_fd, buffer, size, 0, (struct sockaddr *)&addr, sizeof(addr));
			if (r == INVALID && isInprocess())
			{
				ev_io_start(m_loop, &m_write);
				co_await m_write_waiter.suspend();
			}
			co_return r;
		}
		co_return INVALID;
	}
	int io_context::close()
	{
		LOG_CORE("m_fd:%d\n", m_fd);
		__close();
		return 0;
	}

	io_context::io_context() : m_fd(INVALID)
	{
		m_tid = gtid();
		m_loop = cosys::data();
		__init_connect();
	}
	int io_context::__connect(int fd, const char *ip, int port)
	{
		sockaddr_in addr;
		fillAddr(addr, ip, port);
		return ::connect(fd, (sockaddr *)&addr, sizeof(addr));
	}
	void io_context::cb_connect(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
		{
			return;
		}
		io_context *_this = (io_context *)(w->data);
		assert(_this != nullptr);
		_this->__del_connect();
		_this->m_read_waiter.resume();
	}
	void io_context::__init_connect()
	{
		m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
		if (m_fd == INVALID)
		{
			return;
		}
		if (setNoBlock(m_fd, true) < 0)
		{
			::close(m_fd);
			return;
		}
		__add_connect();
	}
	int io_context::__add_connect()
	{
		m_read.data = this;
		ev_io_init(&m_read, &io_context::cb_connect, m_fd, EV_READ | EV_WRITE);
		ev_io_start(m_loop, &m_read);
		return 0;
	}
	int io_context::__del_connect()
	{
		ev_io_stop(m_loop, &m_read);
		return 0;
	}
	int io_context::__del_write()
	{
		if (m_write_waiter.empty())
		{
			ev_io_stop(m_loop, &m_write);
		}
		return 0;
	}
	int io_context::__connect(const char *ip, int port)
	{
		if (__connect(m_fd, ip, port) < 0)
		{
			if (isInprocess())
			{
				return m_fd;
			}
		}
		return __close();
	}
	awaitable<int> io_context::connect(const char *ip, int port)
	{
		m_fd = __connect(ip, port);
		if (m_fd == INVALID)
		{
			co_return m_fd;
		}
		co_await m_read_waiter.suspend();
		auto err = getSocketError(m_fd);
		if (err != 0)
		{
			co_return __close();
		}
		__initial();
		co_return m_fd;
	}
}