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
#include "iocontext.h"

namespace coev
{
	void iocontext::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (iocontext *)w->data;
		assert(_this != NULL);
		_this->m_read_listener.resume();
	}
	void iocontext::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (iocontext *)w->data;
		assert(_this != NULL);
		_this->m_write_listener.resume();
	}
	int iocontext::__close()
	{
		if (m_fd != INVALID)
		{
			__finally();
			::close(m_fd);
			m_fd = INVALID;
			while (m_read_listener.resume())
			{
			}
			while (m_write_listener.resume())
			{
			}
		}
		return 0;
	}
	bool iocontext::__valid() const
	{
		return m_fd != INVALID;
	}
	iocontext::operator bool() const
	{
		return m_fd != INVALID;
	}
	iocontext::iocontext(int fd) : m_fd(fd)
	{
		m_tid = gtid();
		__init();
	}
	int iocontext::__init()
	{
		LOG_CORE("fd:%d\n", m_fd);
		if (m_fd != INVALID)
		{
			m_read.data = this;
			ev_io_init(&m_read, iocontext::cb_read, m_fd, EV_READ);
			ev_io_start(cosys::at(m_tid), &m_read);

			m_write.data = this;
			ev_io_init(&m_write, iocontext::cb_write, m_fd, EV_WRITE);
		}
		return 0;
	}
	int iocontext::__finally()
	{
		LOG_CORE("fd:%d\n", m_fd);
		if (m_fd != INVALID)
		{
			ev_io_stop(cosys::at(m_tid), &m_read);
			ev_io_stop(cosys::at(m_tid), &m_write);
		}
		return 0;
	}
	iocontext::~iocontext()
	{
		if (m_fd != INVALID)
		{
			__finally();
			::close(m_fd);
			m_fd = INVALID;
		}
	}
	awaitable<int> iocontext::send(const char *buffer, int size)
	{
		while (__valid())
		{
			auto r = ::send(m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				ev_io_start(cosys::at(m_tid), &m_write);
				co_await m_write_listener.suspend();
				if (m_write_listener.empty())
				{
					ev_io_stop(cosys::at(m_tid), &m_write);
				}
			}
			else
			{
				co_return r;
			}
		}
		co_return INVALID;
	}
	awaitable<int> iocontext::recv(char *buffer, int size)
	{
		while (__valid())
		{
			co_await m_read_listener.suspend();
			auto r = ::recv(m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				continue;
			}
			co_return r;
		}
		co_return INVALID;
	}
	awaitable<int> iocontext::recvfrom(char *buffer, int size, host &info)
	{
		while (__valid())
		{
			co_await m_read_listener.suspend();
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
	awaitable<int> iocontext::sendto(const char *buffer, int size, host &info)
	{
		while (__valid())
		{
			sockaddr_in addr;
			fillAddr(addr, info.addr, info.port);
			int r = ::sendto(m_fd, buffer, size, 0, (struct sockaddr *)&addr, sizeof(addr));
			if (r == INVALID && isInprocess())
			{
				ev_io_start(cosys::at(m_tid), &m_write);
				co_await m_write_listener.suspend();
				if (m_write_listener.empty())
				{
					ev_io_stop(cosys::at(m_tid), &m_write);
				}
			}
			co_return r;
		}
		co_return INVALID;
	}
	awaitable<int> iocontext::close()
	{
		LOG_CORE("m_fd:%d\n", m_fd);
		__close();
		co_return 0;
	}
}