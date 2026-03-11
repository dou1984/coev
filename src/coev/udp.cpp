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

	int bindfd(const char *ip, int port) noexcept
	{
		int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (fd == INVALID)
		{
			return fd;
		}
		int on = 1;
		if (setReuseAddr(fd, on) < 0)
		{
		__error__:
			::close(fd);
			fd = INVALID;
			return fd;
		}
		if (bindAddr(fd, ip, port) < 0)
		{
			goto __error__;
		}
		if (setNoBlock(fd, true) < 0)
		{
			goto __error__;
		}
		return fd;
	}
	int socketfd() noexcept
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