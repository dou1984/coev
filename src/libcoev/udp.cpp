/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "udp.h"

namespace coev::udp
{
	iocontext bind(const char *ip, int port)
	{
		int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (fd == INVALID)
		{
			return iocontext(fd);
		}
		int on = 1;
		if (setReuseAddr(fd, on) < 0)
		{
		__error_return__:
			::close(fd);
			fd = INVALID;
			return iocontext(fd);
		}
		if (bindAddress(fd, ip, port) < 0)
		{
			goto __error_return__;
		}
		if (setNoBlock(fd, true) < 0)
		{
			goto __error_return__;
		}
		return iocontext(fd);
	}
	iocontext socket()
	{
		int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (fd == INVALID)
		{
			return iocontext(fd);
		}
		if (setNoBlock(fd, true) < 0)
		{
			::close(fd);
			fd = INVALID;
			return iocontext(fd);
		}
		return iocontext(fd);
	}
}