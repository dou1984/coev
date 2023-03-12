#include "Udp.h"

namespace coev
{
	Client Udp::bind(const char *ip, int port)
	{
		int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (fd == INVALID)
		{
			return Client(fd);
		}
		int on = 1;
		if (setReuseAddr(fd, on) < 0)
		{
		__error_return__:
			::close(fd);
			fd = INVALID;
			return Client(fd);
		}
		if (bindAddress(fd, ip, port) < 0)
		{
			goto __error_return__;
		}
		if (setNoBlock(fd, true) < 0)
		{
			goto __error_return__;
		}
		return Client(fd);
	}
	Client Udp::socket()
	{
		int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (fd == INVALID)
		{
			return Client(fd);
		}
		if (setNoBlock(fd, true) < 0)
		{
			::close(fd);
			fd = INVALID;
			return Client(fd);
		}
		return Client(fd);
	}
}