/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <cerrno>
#include "Socket.h"

namespace coev
{
	void SIN_ZERO(void *sin_zero)
	{
		unsigned long long *ptr = (unsigned long long *)sin_zero;
		*ptr = 0;
	}
	void FILL_ADDR(sockaddr_in &addr, const char *ip, int port)
	{
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		inet_pton(addr.sin_family, ip, &addr.sin_addr);
		SIN_ZERO(addr.sin_zero);
	}
	void PARSE_ADDR(sockaddr_in &addr, ipaddress &info)
	{
		inet_ntop(AF_INET, &(addr.sin_addr), info.ip, sizeof(info.ip));
		info.port = ntohs(addr.sin_port);
	}
	int bindLocal(int fd, const char *ip)
	{
		struct sockaddr_un addr;
		addr.sun_family = AF_LOCAL;
		addr.sun_path[0] = '\0';
		strcpy(addr.sun_path + 1, ip);
		return ::bind(fd, (sockaddr *)&addr, sizeof(addr.sun_family) + strlen(ip) + 1);
	}
	int bindAddress(int fd, const char *ip, int port)
	{
		sockaddr_in addr;
		FILL_ADDR(addr, ip, port);
		return ::bind(fd, (sockaddr *)&addr, (int)sizeof(addr));
	}
	int connectLocal(int fd, const char *ip)
	{
		sockaddr_un addr;
		addr.sun_family = AF_LOCAL;
		addr.sun_path[0] = '\0';
		strcpy(addr.sun_path + 1, ip);
		return ::connect(fd, (sockaddr *)&addr, sizeof(addr.sun_family) + strlen(ip) + 1);
	}
	int connectTCP(int fd, const char *ip, int port)
	{
		sockaddr_in addr;
		FILL_ADDR(addr, ip, port);
		return ::connect(fd, (sockaddr *)&addr, sizeof(addr));
	}
	int getSocketError(int fd)
	{
		int error_value = 0;
		socklen_t len = sizeof(socklen_t);
		return getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&error_value, &len) == -1 ? -1 : error_value;
	}
	int acceptTCP(int fd, ipaddress &info)
	{
		sockaddr_in addr;
		socklen_t addr_len = sizeof(sockaddr_in);
		int rfd = accept(fd, (sockaddr *)&addr, &addr_len);
		if (rfd != INVALID)
		{
			PARSE_ADDR(addr, info);
		}
		return rfd;
	}
	int acceptLocal(int fd, ipaddress &info)
	{
		sockaddr_un addr;
		addr.sun_family = AF_LOCAL;
		addr.sun_path[0] = '\0';
		socklen_t addr_len = sizeof(sockaddr_un);
		int rfd = accept(fd, (sockaddr *)&addr, &addr_len);
		if (rfd != INVALID)
		{
			strncpy(info.ip, addr.sun_path, sizeof(info.ip));
		}
		return rfd;
	}
	int setNoBlock(int fd, bool block)
	{
		int ret = 0;
		if (block)
		{
			ret = fcntl(fd, F_SETFL, O_NONBLOCK | fcntl(fd, F_GETFL));
		}
		else
		{
			ret = fcntl(fd, F_SETFL, ~O_NONBLOCK & fcntl(fd, F_GETFL));
		}
		return ret;
	}
	int setKeepAlive(int fd, int alive)
	{
		return setsockopt(fd, IPPROTO_TCP, SO_KEEPALIVE, (char *)&alive, sizeof(alive));
	}
	int setReuseAddr(int fd, int reuse)
	{
		return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	}
	int setSendTimeout(int fd, int _timeout)
	{
		struct timeval timeout = {_timeout, 0};
		socklen_t len = sizeof(timeout);
		return setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
	}
	int setRecvTimeout(int fd, int _timeout)
	{
		struct timeval timeout = {_timeout, 0};
		socklen_t len = sizeof(timeout);
		return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, len);
	}
	int setSendBuffSize(int fd, int bufsize)
	{
		return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
	}
	int setRecvBuffSize(int fd, int bufsize)
	{
		return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
	}
	int getsendbuffsize(int fd, int *bufsize)
	{
		socklen_t len = sizeof(int);
		return getsockopt(fd, SOL_SOCKET, SO_SNDBUF, bufsize, &len);
	}
	int getRecvBuffSize(int fd, int *bufsize)
	{
		socklen_t len = sizeof(int);
		return getsockopt(fd, SOL_SOCKET, SO_RCVBUF, bufsize, &len);
	}
	int setLingerOff(int fd)
	{
		struct linger so_linger;
		so_linger.l_onoff = 0;
		so_linger.l_linger = 0;
		return setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
	}
	int setLingerOn(int fd, uint16_t _timeout)
	{
		struct linger so_linger;
		so_linger.l_onoff = _timeout == 0 ? 0 : 1;
		so_linger.l_linger = _timeout;
		return setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
	}
	int setNoDelay(int fd, int nodelay)
	{
		return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&nodelay, sizeof(nodelay));
	}
	int getSockName(int fd, ipaddress &info)
	{
		struct sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		int ret = ::getsockname(fd, (struct sockaddr *)&addr, &addr_len);
		if (ret == 0)
		{
			inet_ntop(AF_INET, &addr.sin_addr, info.ip, sizeof(info.ip));
			info.port = ntohs(addr.sin_port);
		}
		return ret;
	}
	int getPeerName(int fd, ipaddress &info)
	{
		struct sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		int ret = ::getpeername(fd, (struct sockaddr *)&addr, &addr_len);
		if (ret == 0)
		{
			inet_ntop(AF_INET, &addr.sin_addr, info.ip, sizeof(info.ip));
			info.port = ntohs(addr.sin_port);
		}
		return ret;
	}
	int getHostName(const char *name, ipaddress &info)
	{
		auto host = gethostbyname(name);
		if (!host)
		{
			return INVALID;
		}
		auto addr = (struct in_addr *)host->h_addr_list[0];
		inet_ntop(AF_INET, addr, info.ip, sizeof(info.ip));
		return 0;
	}
	bool isInprocess()
	{
		return errno == EAGAIN || errno == EINPROGRESS || errno == EINTR;
	}
}