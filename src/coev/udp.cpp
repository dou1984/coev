/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "udp.h"
#include "cosys.h"

namespace coev::udp
{

	int bindfd(const char *ip, int port)
	{
		int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (fd == INVALID)
		{
			return fd;
		}
		int on = 1;
		if (setReuseAddr(fd, on) < 0)
		{
		__error_return__:
			::close(fd);
			fd = INVALID;
			return fd;
		}
		if (bindAddr(fd, ip, port) < 0)
		{
			goto __error_return__;
		}
		if (setNoBlock(fd, true) < 0)
		{
			goto __error_return__;
		}
		return fd;
	}
	int socketfd()
	{
		int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (fd == INVALID)
		{
			return fd;
		}
		if (setNoBlock(fd, true) < 0)
		{
			::close(fd);
			fd = INVALID;
			return fd;
		}
		return fd;
	}
}