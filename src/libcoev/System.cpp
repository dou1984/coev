/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <ev.h>
#include "Socket.h"
#include "Loop.h"
#include "System.h"
#include "Timer.h"

namespace coev
{
	extern void SIN_ZERO(void *sin_zero);
	extern void FILL_ADDR(sockaddr_in &addr, const char *ip, int port);
	extern void PARSE_ADDR(sockaddr_in &addr, ipaddress &info);

	Awaiter<int> accept(Server &_server, ipaddress &peer)
	{
		if (!_server)
		{
			co_return INVALID;
		}
		co_await wait_for<EVRecv>(_server);
		auto fd = acceptTCP(_server.m_fd, peer);
		if (fd != INVALID)
		{
			setNoBlock(fd, true);
		}
		co_return fd;
	}
	Awaiter<int> connect(Client &c, const char *ip, int port)
	{
		int fd = c.connect(ip, port);
		if (fd == INVALID)
		{
			co_return fd;
		}
		co_await wait_for<EVRecv>(c);
		auto err = getSocketError(c.m_fd);
		if (err == 0)
		{
			c.__init();
			co_return fd;
		}
		co_return c.close();
	}
	Awaiter<int> send(IOContext &_client, const char *buffer, int size)
	{
		while (_client)
		{
			auto r = ::send(_client.m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				ev_io_start(Loop::at(_client.m_tag), &_client.m_Write);
				co_await wait_for<EVSend>(_client);
				if (_client.EVSend::empty())
					ev_io_stop(Loop::at(_client.m_tag), &_client.m_Write);
			}
			else
			{
				co_return r;
			}
		}
		co_return INVALID;
	}
	Awaiter<int> recv(IOContext &_client, char *buffer, int size)
	{
		while (_client)
		{
			co_await wait_for<EVRecv>(_client);
			auto r = ::recv(_client.m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				continue;
			}
			co_return r;
		}
		co_return INVALID;
	}
	Awaiter<int> recvfrom(IOContext &_client, char *buffer, int size, ipaddress &info)
	{
		while (_client)
		{
			co_await wait_for<EVRecv>(_client);
			sockaddr_in addr;
			socklen_t addrsize = sizeof(addr);
			int r = ::recvfrom(_client.m_fd, buffer, size, 0, (struct sockaddr *)&addr, &addrsize);
			if (r == INVALID && isInprocess())
			{
				continue;
			}
			PARSE_ADDR(addr, info);
			co_return r;
		}
		co_return INVALID;
	}
	Awaiter<int> sendto(IOContext &_client, const char *buffer, int size, ipaddress &info)
	{
		while (_client)
		{
			sockaddr_in addr;
			FILL_ADDR(addr, info.ip, info.port);
			int r = ::sendto(_client.m_fd, buffer, size, 0, (struct sockaddr *)&addr, sizeof(addr));
			if (r == INVALID && isInprocess())
			{
				ev_io_start(Loop::at(_client.m_tag), &_client.m_Write);
				co_await wait_for<EVSend>(_client);
				if (_client.EVSend::empty())
					ev_io_stop(Loop::at(_client.m_tag), &_client.m_Write);
			}
			co_return r;
		}
		co_return INVALID;
	}
	Awaiter<int> close(IOContext &_client)
	{
		_client.close();
		co_return 0;
	}
	Awaiter<int> sleep_for(long t)
	{
		Timer _timer(t, 0);
		_timer.active();
		co_await wait_for<EVTimer>(_timer);
		co_return 0;
	}
	Awaiter<int> usleep_for(long t)
	{
		Timer _timer((float)t / 1000000, 0);
		_timer.active();
		co_await wait_for<EVTimer>(_timer);
		co_return 0;
	}
}