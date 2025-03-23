/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "server.h"
#include "coev.h"
#include "cosys.h"

namespace coev::tcp
{
	static int __real_accept(int fd, addrInfo &info)
	{
		sockaddr_in addr;
		socklen_t addr_len = sizeof(sockaddr_in);
		int rfd = ::accept(fd, (sockaddr *)&addr, &addr_len);
		if (rfd != INVALID)
		{
			parseAddr(addr, info);
		}
		return rfd;
	}
	void server::cb_accept(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
		{
			return;
		}
		server *_this = (server *)(w->data);
		assert(_this != nullptr);
		LOG_CORE("server::cb_accept %p\n", _this);
		_this->m_waiter.resume(true);
		local<coev::async>::instance().resume_all();
	}
	server::server()
	{
		m_loop = cosys::data();
	}
	server::~server()
	{
		stop();
	}
	int server::start(const char *ip, int port)
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
		if (bindAddr(m_fd, ip, port) < 0)
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
		__insert();
		return m_fd;
	}
	int server::stop()
	{
		if (m_fd != INVALID)
		{
			__remove();
			::close(m_fd);
			m_fd = INVALID;
		}
		return 0;
	}
	int server::insert(int fd)
	{
		m_fd = fd;
		return __insert();
	}
	int server::__insert()
	{
		LOG_CORE("ev_io_start:%p %p\n", m_loop, &m_recv);
		m_recv.data = this;
		ev_io_init(&m_recv, server::cb_accept, m_fd, EV_READ);
		ev_io_start(m_loop, &m_recv);
		return m_fd;
	}
	int server::__remove()
	{
		ev_io_stop(m_loop, &m_recv);
		return m_fd;
	}

	awaitable<int> server::accept(addrInfo &peer)
	{
		if (!valid())
		{
			co_return INVALID;
		}
		co_await m_waiter.suspend();
		co_return __real_accept(m_fd, peer);
	}

}