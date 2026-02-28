/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <cerrno>
#include <string.h>
#include <stdio.h>
#include "socket.h"
#include "invalid.h"

namespace coev
{
	addrInfo::addrInfo(const addrInfo &o)
	{
		*this = o;
	}
	addrInfo::addrInfo(const char *_ip, int _port)
	{
		constexpr int l = sizeof(ip);
		strncpy(ip, _ip, l);
		ip[l - 1] = '\0';
		port = _port;
	}
	const addrInfo &addrInfo::operator=(const addrInfo &o)
	{
		constexpr int l = sizeof(ip);
		strncpy(ip, o.ip, l);
		ip[l - 1] = '\0';
		port = o.port;
		return *this;
	}
	int addrInfo::fromUrl(const char *url)
	{
		auto count = sscanf(url, "%[^:]:%d", ip, &port);
		switch (count)
		{
		case 2:
		case 1:
			return 0;
		default:
			return -1;
		}
	}
	void zeroSin(void *sin_zero)
	{
		unsigned long long *ptr = (unsigned long long *)sin_zero;
		*ptr = 0;
	}
	void fillAddr(sockaddr_in &addr, const char *ip, int port)
	{
		assert(ip);
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		inet_pton(addr.sin_family, ip, &addr.sin_addr);
		zeroSin(addr.sin_zero);
	}
	void parseAddr(sockaddr_in &addr, addrInfo &info)
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
	int bindAddr(int fd, const char *ip, int port)
	{
		sockaddr_in addr;
		fillAddr(addr, ip, port);
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
	int getSocketError(int fd)
	{
		int error_value = 0;
		socklen_t len = sizeof(socklen_t);
		return getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&error_value, &len) == -1 ? -1 : error_value;
	}
	int acceptLocal(int fd, addrInfo &info)
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
	int getSockName(int fd, addrInfo &info)
	{
		struct sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		int ret = ::getsockname(fd, (struct sockaddr *)&addr, &addr_len);
		if (ret == 0)
		{
			parseAddr(addr, info);
		}
		return ret;
	}
	int getPeerName(int fd, addrInfo &info)
	{
		struct sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		int ret = ::getpeername(fd, (struct sockaddr *)&addr, &addr_len);
		if (ret == 0)
		{
			parseAddr(addr, info);
		}
		return ret;
	}
	int getHostName(const char *name, addrInfo &info)
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