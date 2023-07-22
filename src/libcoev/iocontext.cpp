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
#include "waitfor.h"

namespace coev
{
	void iocontext::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (iocontext *)w->data;
		assert(_this != NULL);
		_this->EVRecv::resume();
	}
	void iocontext::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (iocontext *)w->data;
		assert(_this != NULL);
		_this->EVSend::resume();
	}
	int iocontext::__close()
	{
		if (m_fd != INVALID)
		{
			__finally();
			::close(m_fd);
			m_fd = INVALID;
			while (EVRecv::resume())
			{
			}
			while (EVSend::resume())
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
		m_tag = ttag();
		__init();
	}
	int iocontext::__init()
	{
		LOG_DBG("fd:%d\n", m_fd);
		if (m_fd != INVALID)
		{
			m_Read.data = this;
			ev_io_init(&m_Read, iocontext::cb_read, m_fd, EV_READ);
			ev_io_start(loop::at(m_tag), &m_Read);

			m_Write.data = this;
			ev_io_init(&m_Write, iocontext::cb_write, m_fd, EV_WRITE);
		}
		return 0;
	}
	int iocontext::__finally()
	{
		LOG_DBG("fd:%d\n", m_fd);
		if (m_fd != INVALID)
		{
			ev_io_stop(loop::at(m_tag), &m_Read);
			ev_io_stop(loop::at(m_tag), &m_Write);
		}
		return 0;
	}
	iocontext::~iocontext()
	{
		assert(EVRecv::empty());
		assert(EVSend::empty());
		if (m_fd != INVALID)
		{
			__finally();
			::close(m_fd);
			m_fd = INVALID;
		}
	}
	const iocontext &iocontext::operator=(iocontext &&o)
	{
		m_tag = ttag();
		o.__finally();
		std::swap(m_fd, o.m_fd);
		o.EVRecv::moveto(static_cast<EVRecv *>(this));
		o.EVSend::moveto(static_cast<EVSend *>(this));
		__init();
		return *this;
	}

	awaiter iocontext::send(const char *buffer, int size)
	{
		while (__valid())
		{
			auto r = ::send(m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				ev_io_start(loop::at(m_tag), &m_Write);
				co_await wait_for<EVSend>(*this);
				if (EVSend::empty())
					ev_io_stop(loop::at(m_tag), &m_Write);
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
			co_await wait_for<EVRecv>(*this);
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
			co_await wait_for<EVRecv>(*this);
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
				ev_io_start(loop::at(m_tag), &m_Write);
				co_await wait_for<EVSend>(*this);
				if (EVSend::empty())
					ev_io_stop(loop::at(m_tag), &m_Write);
			}
			co_return r;
		}
		co_return INVALID;
	}
	awaiter iocontext::close()
	{
		TRACE();
		__close();
		co_return 0;
	}
}