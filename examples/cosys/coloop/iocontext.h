/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <ev.h>
#include <memory>
#include <coev.h>
#include "socket.h"

namespace coev
{
	class iocontext
	{
	public:
		iocontext() = default;
		iocontext(int fd);
		iocontext(iocontext &&) = delete;
		virtual ~iocontext();
		awaiter send(const char *, int);
		awaiter recv(char *, int);
		awaiter recvfrom(char *, int, ipaddress &);
		awaiter sendto(const char *, int, ipaddress &);
		awaiter close();
		operator bool() const;

	protected:
		uint64_t m_tid = 0;
		int m_fd = INVALID;
		ev_io m_Read;
		ev_io m_Write;

		trigger m_trigger_read;
		trigger m_trigger_write;
		int __finally();
		int __init();
		int __close();
		bool __valid() const;
		static void cb_write(struct ev_loop *loop, struct ev_io *w, int revents);
		static void cb_read(struct ev_loop *loop, struct ev_io *w, int revents);
	};
}
