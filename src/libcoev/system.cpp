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
	awaiter<sharedIOContext> accept(tcp::server &_server, ipaddress &peer)
	{
		if (!_server)
		{
			co_return std::make_shared<iocontext>(INVALID);
		}
		co_await wait_for<EVRecv>(_server);
		auto fd = __accept(_server.m_fd, peer);
		if (fd != INVALID)
		{
			setNoBlock(fd, true);
		}
		co_return std::make_shared<iocontext>(fd);
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