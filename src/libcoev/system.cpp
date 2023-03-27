/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <ev.h>
#include "Socket.h"
#include "loop.h"
#include "System.h"
#include "timer.h"

namespace coev
{
	extern void SIN_ZERO(void *sin_zero);
	extern void FILL_ADDR(sockaddr_in &addr, const char *ip, int port);
	extern void PARSE_ADDR(sockaddr_in &addr, ipaddress &info);

	awaiter<SharedIO> accept(tcp::server &_server, ipaddress &peer)
	{
		if (!_server)
		{
			co_return std::make_shared<iocontext>(INVALID);
		}
		co_await wait_for<EVRecv>(_server);
		auto fd = acceptTCP(_server.m_fd, peer);
		if (fd != INVALID)
		{
			setNoBlock(fd, true);
		}
		co_return std::make_shared<iocontext>(fd);
	}
	awaiter<int> connect(client &c, const char *ip, int port)
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
	awaiter<int> send(iocontext &io, const char *buffer, int size)
	{
		while (io)
		{
			auto r = ::send(io.m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				ev_io_start(loop::at(io.m_tag), &io.m_Write);
				co_await wait_for<EVSend>(io);
				if (io.EVSend::empty())
					ev_io_stop(loop::at(io.m_tag), &io.m_Write);
			}
			else
			{
				co_return r;
			}
		}
		co_return INVALID;
	}
	awaiter<int> recv(iocontext &io, char *buffer, int size)
	{
		while (io)
		{
			co_await wait_for<EVRecv>(io);
			auto r = ::recv(io.m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				continue;
			}
			co_return r;
		}
		co_return INVALID;
	}
	awaiter<int> recvfrom(iocontext &io, char *buffer, int size, ipaddress &info)
	{
		while (io)
		{
			co_await wait_for<EVRecv>(io);
			sockaddr_in addr;
			socklen_t addrsize = sizeof(addr);
			int r = ::recvfrom(io.m_fd, buffer, size, 0, (struct sockaddr *)&addr, &addrsize);
			if (r == INVALID && isInprocess())
			{
				continue;
			}
			PARSE_ADDR(addr, info);
			co_return r;
		}
		co_return INVALID;
	}
	awaiter<int> sendto(iocontext &io, const char *buffer, int size, ipaddress &info)
	{
		while (io)
		{
			sockaddr_in addr;
			FILL_ADDR(addr, info.ip, info.port);
			int r = ::sendto(io.m_fd, buffer, size, 0, (struct sockaddr *)&addr, sizeof(addr));
			if (r == INVALID && isInprocess())
			{
				ev_io_start(loop::at(io.m_tag), &io.m_Write);
				co_await wait_for<EVSend>(io);
				if (io.EVSend::empty())
					ev_io_stop(loop::at(io.m_tag), &io.m_Write);
			}
			co_return r;
		}
		co_return INVALID;
	}
	awaiter<int> close(iocontext &io)
	{
		io.close();
		co_return 0;
	}
	awaiter<int> sleep_for(long t)
	{
		timer _timer(t, 0);
		_timer.active();
		co_await wait_for<EVTimer>(_timer);
		co_return 0;
	}
	awaiter<int> usleep_for(long t)
	{
		timer _timer((float)t / 1000000, 0);
		_timer.active();
		co_await wait_for<EVTimer>(_timer);
		co_return 0;
	}
}