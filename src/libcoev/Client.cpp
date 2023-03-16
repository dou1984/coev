/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "Client.h"
#include "Loop.h"

namespace coev
{
	void Client::cb_connect(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
		{
			return;
		}
		Client *_this = (Client *)(w->data);
		assert(_this != nullptr);
		assert(*_this);
		_this->connect_remove();
		_this->EVRecv::resume_ex();
	}
	Client::Client()
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
		m_tag = Loop::tag();
		connect_insert();
	}
	Client::~Client()
	{
		//assert(EVConnect::empty());
		close();
	}
	int Client::connect_insert()
	{
		m_Read.data = this;
		ev_io_init(&m_Read, &Client::cb_connect, m_fd, EV_READ | EV_WRITE);
		ev_io_start(Loop::at(m_tag), &m_Read);
		return 0;
	}
	int Client::connect_remove()
	{
		ev_io_stop(Loop::at(m_tag), &m_Read);
		return 0;
	}
	int Client::connect(const char *ip, int port)
	{
		if (connectTCP(m_fd, ip, port) < 0)
		{
			if (isInprocess())
			{
				return m_fd;
			}
		}
		return close();
	}
}