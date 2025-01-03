/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "server.h"
#include "../coev/coev.h"
#include "cosys.h"

namespace coev::tcp
{
	static int __real_accept(int fd, host &info)
	{
		sockaddr_in addr;
		socklen_t addr_len = sizeof(sockaddr_in);
		int rfd = accept(fd, (sockaddr *)&addr, &addr_len);
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
		_this->m_listener.resume();
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
		return m_fd;
	}
	int server::stop()
	{
		if (m_fd != INVALID)
		{
			__remove(gtid());
			::close(m_fd);
			m_fd = INVALID;
		}
		return 0;
	}
	int server::__insert(uint64_t _tid)
	{
		m_reav.data = this;
		ev_io_init(&m_reav, server::cb_accept, m_fd, EV_READ);
		ev_io_start(cosys::at(_tid), &m_reav);
		return m_fd;
	}
	int server::__remove(uint64_t _tid)
	{
		ev_io_stop(cosys::at(_tid), &m_reav);
		return m_fd;
	}
	bool server::__valid() const
	{
		return m_fd != INVALID;
	}
	awaitable<int> server::accept(host &peer)
	{
		if (!__valid())
		{
			co_return INVALID;
		}
		co_await m_listener.suspend();
		auto fd = __real_accept(m_fd, peer);
		if (fd != INVALID)
		{
			setNoBlock(fd, true);
		}
		co_return fd;
	}

}