/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "client.h"
#include "cosys.h"

namespace coev
{
	int client::__connect(int fd, const char *ip, int port)
	{
		sockaddr_in addr;
		fillAddr(addr, ip, port);
		return ::connect(fd, (sockaddr *)&addr, sizeof(addr));
	}
	void client::cb_connect(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
		{
			return;
		}
		client *_this = (client *)(w->data);
		assert(_this != nullptr);
		_this->__remove();
		_this->m_read_waiter.resume();
	}
	client::client() : io_context(INVALID)
	{
		__initial();
	}
	client::~client()
	{
		__close();
	}
	void client::__initial()
	{
		m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
		if (m_fd == INVALID)
		{
			return;
		}
		if (setNoBlock(m_fd, true) < 0)
		{
			::close(m_fd);
			return;
		}
		__insert();
	}
	int client::__insert()
	{
		m_read.data = this;
		ev_io_init(&m_read, &client::cb_connect, m_fd, EV_READ | EV_WRITE);
		ev_io_start(m_loop, &m_read);
		return 0;
	}
	int client::__remove()
	{
		ev_io_stop(m_loop, &m_read);
		return 0;
	}
	int client::__connect(const char *ip, int port)
	{
		if (__connect(m_fd, ip, port) < 0)
		{
			if (isInprocess())
			{
				return m_fd;
			}
		}
		return __close();
	}
	awaitable<int> client::connect(const char *ip, int port)
	{
		m_fd = __connect(ip, port);
		if (m_fd == INVALID)
		{
			co_return m_fd;
		}
		co_await m_read_waiter.suspend();
		auto err = getSocketError(m_fd);
		if (err == 0)
		{
			__init();
			co_return m_fd;
		}
		co_return __close();
	}
}