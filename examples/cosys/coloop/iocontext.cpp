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
#include "loop.h"
#include "iocontext.h"

namespace coev
{
	template <size_t I = 0, async_t... T>
	auto &get_async(async<T...> *_this)
	{
		return std::get<I>(*_this);
	}
	void iocontext::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (iocontext *)w->data;
		assert(_this != NULL);
		resume<0>(_this);
	}
	void iocontext::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (iocontext *)w->data;
		assert(_this != NULL);
		resume<1>(_this);
	}
	int iocontext::__close()
	{
		if (m_fd != INVALID)
		{
			__finally();
			::close(m_fd);
			m_fd = INVALID;
			while (resume<0>(this))
			{
			}
			while (resume<1>(this))
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
			m_Read.data = this;
			ev_io_init(&m_Read, iocontext::cb_read, m_fd, EV_READ);
			ev_io_start(loop::at(m_tid), &m_Read);

			m_Write.data = this;
			ev_io_init(&m_Write, iocontext::cb_write, m_fd, EV_WRITE);
		}
		return 0;
	}
	int iocontext::__finally()
	{
		LOG_CORE("fd:%d\n", m_fd);
		if (m_fd != INVALID)
		{
			ev_io_stop(loop::at(m_tid), &m_Read);
			ev_io_stop(loop::at(m_tid), &m_Write);
		}
		return 0;
	}
	iocontext::~iocontext()
	{
		assert(get_async<0>(this).empty());
		assert(get_async<1>(this).empty());

		if (m_fd != INVALID)
		{
			__finally();
			::close(m_fd);
			m_fd = INVALID;
		}
	}
	awaiter iocontext::send(const char *buffer, int size)
	{
		while (__valid())
		{
			auto r = ::send(m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				ev_io_start(loop::at(m_tid), &m_Write);
				co_await wait_for<1>(this);
				if (get_async<1>(this).empty())
					ev_io_stop(loop::at(m_tid), &m_Write);
			}
			else
			{
				co_return r;
			}
		}
		co_return INVALID;
	}
	awaiter iocontext::recv(char *buffer, int size)
	{
		while (__valid())
		{
			co_await wait_for<0>(this);
			auto r = ::recv(m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				continue;
			}
			co_return r;
		}
		co_return INVALID;
	}
	awaiter iocontext::recvfrom(char *buffer, int size, ipaddress &info)
	{
		while (__valid())
		{
			co_await wait_for<0>(this);
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
	awaiter iocontext::sendto(const char *buffer, int size, ipaddress &info)
	{
		while (__valid())
		{
			sockaddr_in addr;
			fillAddr(addr, info.ip, info.port);
			int r = ::sendto(m_fd, buffer, size, 0, (struct sockaddr *)&addr, sizeof(addr));
			if (r == INVALID && isInprocess())
			{
				ev_io_start(loop::at(m_tid), &m_Write);
				co_await wait_for<1>(this);
				if (get_async<1>(this).empty())
					ev_io_stop(loop::at(m_tid), &m_Write);
			}
			co_return r;
		}
		co_return INVALID;
	}
	awaiter iocontext::close()
	{
		LOG_CORE("m_fd:%d\n", m_fd);
		__close();
		co_return 0;
	}
}