#include "Udp.h"

namespace coev
{
	IOContext Udp::bind(const char *ip, int port)
	{
		int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (fd == INVALID)
		{
			return IOContext(fd);
		}
		int on = 1;
		if (setReuseAddr(fd, on) < 0)
		{
		__error_return__:
			::close(fd);
			fd = INVALID;
			return IOContext(fd);
		}
		if (bindAddress(fd, ip, port) < 0)
		{
			goto __error_return__;
		}
		if (setNoBlock(fd, true) < 0)
		{
			goto __error_return__;
		}
		return IOContext(fd);
	}
	IOContext Udp::socket()
	{
		int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (fd == INVALID)
		{
			return IOContext(fd);
		}
		if (setNoBlock(fd, true) < 0)
		{
			::close(fd);
			fd = INVALID;
			return IOContext(fd);
		}
		return IOContext(fd);
	}
}