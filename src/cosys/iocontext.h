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
#include <coev/coev.h>
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
		awaitable<int> send(const char *, int);
		awaitable<int> recv(char *, int);
		awaitable<int> recvfrom(char *, int, host &);
		awaitable<int> sendto(const char *, int, host &);
		awaitable<int> close();
		operator bool() const;

	protected:
		uint64_t m_tid = 0;
		struct ev_loop *m_loop = nullptr;
		int m_fd = INVALID;
		ev_io m_read;
		ev_io m_write;

		async m_read_waiter;
		async m_write_waiter;
		int __finally();
		int __init();
		int __close();
		bool __valid() const;
		static void cb_write(struct ev_loop *loop, struct ev_io *w, int revents);
		static void cb_read(struct ev_loop *loop, struct ev_io *w, int revents);
	};
}
