/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define INVALID (-1)

namespace coev
{
	struct addrInfo
	{
		char ip[64];
		int port;
		addrInfo() = default;
		addrInfo(const addrInfo &o);
		addrInfo(const char *, int);
		const addrInfo &operator=(const addrInfo &);
		int fromUrl(const char *url);

	};

	int bindLocal(int fd, const char *ip);

	int bindAddr(int fd, const char *ip, int port);

	int connectLocal(int fd, const char *ip);

	int acceptLocal(int fd, addrInfo &);

	int getSocketError(int fd);

	int setNoBlock(int fd, bool block);

	int setKeepAlive(int fd, int alive);

	int setReuseAddr(int fd, int reuse);

	int setSendTimeout(int fd, int _timeout);

	int setRecvTimeout(int fd, int _timeout);

	int setSendBuffSize(int fd, int bufsize);

	int setRecvBuffSize(int fd, int bufsize);

	int getsendbuffsize(int fd, int *bufsize);

	int getRecvBuffSize(int fd, int *bufsize);

	int setLingerOff(int fd);

	int setLingerOn(int fd, uint16_t _timeout);

	int setNoDelay(int fd, int nodelay);

	int getSockName(int fd, addrInfo &);

	int getPeerName(int fd, char *ip, int len, int &port);

	int getHostName(const char *name, char *ip, int len);

	bool isInprocess();

	void fillAddr(sockaddr_in &addr, const char *ip, int port);

	void parseAddr(sockaddr_in &addr, addrInfo &info);
}