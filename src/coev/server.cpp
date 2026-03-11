/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "server.h"
#include "coev.h"
#include "cosys.h"
#include "co_task.h"
#include "local_resume.h"

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
		LOG_CORE("server::cb_accept %p", _this);
		_this->m_waiter.resume();
		local_resume();
	}
	server::server()
	{
		m_loop = cosys::data();
		co_start << [this]() -> awaitable<void>
		{
			co_await m_finished.lock();
			__stop();
		}();
	}
	server::~server()
	{
		m_finished.unlock();
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
		__error__:
			auto _fd = std::exchange(m_fd, INVALID);
			::close(_fd);
			return m_fd;
		}
		if (bindAddr(m_fd, ip, port) < 0)
		{
			goto __error__;
		}
		if (setNoBlock(m_fd, true) < 0)
		{
			goto __error__;
		}
		if (listen(m_fd, 1000) < 0)
		{
			goto __error__;
		}
		__insert();
		return m_fd;
	}
	int server::bind(int fd)
	{
		m_fd = fd;
		return __insert();
	}
	int server::__insert()
	{
		LOG_CORE("ev_io_start:%p %p", m_loop, &m_recv);
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
	int server::__stop()
	{
		if (m_fd != INVALID)
		{
			auto _fd = std::exchange(m_fd, INVALID);
			__remove();
			::close(_fd);
		}
		return 0;
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