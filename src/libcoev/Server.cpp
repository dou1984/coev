/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "Server.h"
#include "Loop.h"

namespace coev
{
	void Server::cb_accept(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		Server *_this = (Server *)(w->data);
		assert(_this != nullptr);
		assert(*_this);
		_this->EVRecv::resume_ex();
	}
	Server::~Server()
	{
		stop();
	}
	Server::operator bool() const
	{
		return m_fd != INVALID;
	}
	int Server::start(const char *ip, int port)
	{
		assert(m_fd == INVALID);
		m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
		if (m_fd == INVALID)
		{
			return m_fd;
		}
		int on = 1;
		if (setReuseAddr(m_fd, on) < 0)
		{
		__error_return__:
			::close(m_fd);
			m_fd = INVALID;
			return m_fd;
		}
		if (bindAddress(m_fd, ip, port) < 0)
		{
			goto __error_return__;
		}
		if (setNoBlock(m_fd, true) < 0)
		{
			goto __error_return__;
		}
		if (listen(m_fd, 1000) < 0)
		{
			goto __error_return__;
		}
		__insert(Loop::tag());
		return m_fd;
	}
	int Server::stop()
	{
		if (m_fd != INVALID)
		{
			__remove(Loop::tag());
			::close(m_fd);
			m_fd = INVALID;
		}
		return 0;
	}
	int Server::__insert(uint32_t _tag)
	{
		m_Reav.data = this;
		ev_io_init(&m_Reav, Server::cb_accept, m_fd, EV_READ);
		ev_io_start(Loop::at(_tag), &m_Reav);
		return m_fd;
	}
	int Server::__remove(uint32_t _tag)
	{
		ev_io_stop(Loop::at(_tag), &m_Reav);
		return m_fd;
	}
}