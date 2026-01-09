/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <ev.h>
#include "socket.h"
#include "io_context.h"

namespace coev::tcp
{	
	class server
	{
	public:
		server();
		virtual ~server();
		int start(const char *ip, int port);
		int stop();
		int bind(int fd);
		awaitable<int> accept(addrInfo &);
		bool valid() const { return m_fd != INVALID; }

	public:
		int __insert();
		int __remove();

	private:
		int m_fd = INVALID;
		struct ev_loop *m_loop = nullptr;
		ev_io m_recv;
		async m_waiter;

		static void cb_accept(struct ev_loop *loop, struct ev_io *w, int revents);
	};
}