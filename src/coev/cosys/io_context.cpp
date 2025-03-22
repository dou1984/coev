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
		_this->m_read_waiter.resume(true);
		local<coev::async>::instance().resume_all();
	}
	void io_context::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (io_context *)w->data;
		assert(_this != NULL);
		_this->m_write_waiter.resume(true);
		local<coev::async>::instance().resume_all();
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
	io_context::io_context() : m_fd(INVALID)
	{
		m_tid = gtid();
		m_loop = cosys::data();		
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
		if (m_fd != INVALID)
		{
			LOG_CORE("fd:%d\n", m_fd);
			ev_io_stop(m_loop, &m_read);
			ev_io_stop(m_loop, &m_write);
		}
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
				LOG_CORE("fd:%d send %ld bytes\n", m_fd, r);
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
			LOG_CORE("fd:%d recv %ld bytes\n", m_fd, r);
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

	

}