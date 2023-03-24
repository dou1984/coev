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
#include "Loop.h"
#include "IOContext.h"

namespace coev
{
	void IOContext::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (IOContext *)w->data;
		assert(_this != NULL);
		assert(*_this);
		_this->EVRecv::resume_ex();
	}
	void IOContext::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (IOContext *)w->data;
		assert(_this != NULL);
		assert(*_this);
		_this->EVSend::resume_ex();
	}
	int IOContext::close()
	{
		if (m_fd != INVALID)
		{
			TRACE();
			__finally();
			::close(m_fd);
			m_fd = INVALID;
			while (EVRecv::resume_ex())
			{
			}
			while (EVSend::resume_ex())
			{
			}
		}
		return 0;
	}
	IOContext::operator bool() const
	{
		return m_fd != INVALID;
	}
	IOContext::IOContext(int fd) : m_fd(fd)
	{
		m_tag = Loop::tag();
		__init();
	}
	int IOContext::__init()
	{
		if (m_fd != INVALID)
		{
			m_Read.data = this;
			ev_io_init(&m_Read, IOContext::cb_read, m_fd, EV_READ);
			ev_io_start(Loop::at(m_tag), &m_Read);

			m_Write.data = this;
			ev_io_init(&m_Write, IOContext::cb_write, m_fd, EV_WRITE);
		}
		return 0;
	}
	int IOContext::__finally()
	{
		if (m_fd != INVALID)
		{
			ev_io_stop(Loop::at(m_tag), &m_Read);
			ev_io_stop(Loop::at(m_tag), &m_Write);
		}
		return 0;
	}
	IOContext::~IOContext()
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
	const IOContext &IOContext::operator=(IOContext &&o)
	{
		m_tag = Loop::tag();
		o.__finally();
		std::swap(m_fd, o.m_fd);
		o.EVRecv::moveto(static_cast<EVRecv *>(this));
		o.EVSend::moveto(static_cast<EVSend *>(this));
		__init();
		return *this;
	}
}