/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "client.h"
#include "loop.h"
#include "system.h"

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
		_this->connect_remove();
		_this->EVRecv::resume();
	}
	client::client()
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
		m_tag = loop::tag();
		connect_insert();
	}
	client::~client()
	{
		close();
	}
	int client::connect_insert()
	{
		m_Read.data = this;
		ev_io_init(&m_Read, &client::cb_connect, m_fd, EV_READ | EV_WRITE);
		ev_io_start(loop::at(m_tag), &m_Read);
		return 0;
	}
	int client::connect_remove()
	{
		ev_io_stop(loop::at(m_tag), &m_Read);
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
	awaiter<int> client::connect(const char *ip, int port)
	{
		int fd = __connect(ip, port);
		if (fd == INVALID)
		{
			co_return fd;
		}
		co_await wait_for<EVRecv>(*this);
		auto err = getSocketError(m_fd);
		if (err == 0)
		{
			__init();
			co_return fd;
		}
		co_return __close();
	}
}