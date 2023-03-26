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
#include "IOContext.h"

namespace coev
{
	void iocontext::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (iocontext *)w->data;
		assert(_this != NULL);
		assert(*_this);
		_this->EVRecv::resume_ex();
	}
	void iocontext::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (iocontext *)w->data;
		assert(_this != NULL);
		assert(*_this);
		_this->EVSend::resume_ex();
	}
	int iocontext::close()
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
	iocontext::operator bool() const
	{
		return m_fd != INVALID;
	}
	iocontext::iocontext(int fd) : m_fd(fd)
	{
		m_tag = loop::tag();
		__init();
	}
	int iocontext::__init()
	{
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
		m_tag = loop::tag();
		o.__finally();
		std::swap(m_fd, o.m_fd);
		o.EVRecv::moveto(static_cast<EVRecv *>(this));
		o.EVSend::moveto(static_cast<EVSend *>(this));
		__init();
		return *this;
	}
}