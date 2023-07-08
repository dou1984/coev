/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "server.h"
#include "loop.h"
#include "system.h"

namespace coev::tcp
{
	static int __accept(int fd, ipaddress &info)
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
			return;
		server *_this = (server *)(w->data);
		assert(_this != nullptr);
		_this->EVRecv::resume_ex();
	}
	server::~server()
	{
		stop();
	}
	server::operator bool() const
	{
		return m_fd != INVALID;
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
		__insert(loop::tag());
		return m_fd;
	}
	int server::stop()
	{
		if (m_fd != INVALID)
		{
			__remove(loop::tag());
			::close(m_fd);
			m_fd = INVALID;
		}
		return 0;
	}
	int server::__insert(uint32_t _tag)
	{
		m_Reav.data = this;
		ev_io_init(&m_Reav, server::cb_accept, m_fd, EV_READ);
		ev_io_start(loop::at(_tag), &m_Reav);
		return m_fd;
	}
	int server::__remove(uint32_t _tag)
	{
		ev_io_stop(loop::at(_tag), &m_Reav);
		return m_fd;
	}
	bool server::__valid() const
	{
		return m_fd != INVALID;
	}
	awaiter<int> server::accept(const fnaccept &__func)
	{
		if (!__valid())
		{
			co_return INVALID;
		}
		co_await wait_for<EVRecv>(*this);
		ipaddress peer;
		auto fd = __accept(m_fd, peer);
		if (fd != INVALID)
		{
			setNoBlock(fd, true);
		}

		[=]() -> awaiter<int>
		{
			iocontext ctx(fd);
			co_await __func(peer, ctx);
			co_return 0;
		}();
		co_return fd;
	}

}