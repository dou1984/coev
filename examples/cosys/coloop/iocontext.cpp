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
#include "coloop.h"
#include "iocontext.h"

namespace coev
{

	void iocontext::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (iocontext *)w->data;
		assert(_this != NULL);
		notify(_this->m_trigger_read);
	}
	void iocontext::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (iocontext *)w->data;
		assert(_this != NULL);
		notify(_this->m_trigger_write);
	}
	int iocontext::__close()
	{
		if (m_fd != INVALID)
		{
			__finally();
			::close(m_fd);
			m_fd = INVALID;
			while (notify(m_trigger_read))
			{
			}
			while (notify(m_trigger_write))
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
			ev_io_start(coloop::at(m_tid), &m_Read);

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
			ev_io_stop(coloop::at(m_tid), &m_Read);
			ev_io_stop(coloop::at(m_tid), &m_Write);
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
				ev_io_start(coloop::at(m_tid), &m_Write);
				co_await wait_for(m_trigger_write);
				if (m_trigger_write.empty())
					ev_io_stop(coloop::at(m_tid), &m_Write);
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
			co_await wait_for(m_trigger_read);
			auto r = ::recv(m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				continue;
			}
			co_return r;
		}
		co_return INVALID;
	}
	awaitable<int> iocontext::recvfrom(char *buffer, int size, ipaddress &info)
	{
		while (__valid())
		{
			co_await wait_for(m_trigger_read);
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
	awaitable<int> iocontext::sendto(const char *buffer, int size, ipaddress &info)
	{
		while (__valid())
		{
			sockaddr_in addr;
			fillAddr(addr, info.ip, info.port);
			int r = ::sendto(m_fd, buffer, size, 0, (struct sockaddr *)&addr, sizeof(addr));
			if (r == INVALID && isInprocess())
			{
				ev_io_start(coloop::at(m_tid), &m_Write);
				co_await wait_for(m_trigger_write);
				if (m_trigger_write.empty())
					ev_io_stop(coloop::at(m_tid), &m_Write);
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