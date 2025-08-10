/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
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
		int insert(int fd);
		awaitable<int> accept(addrInfo &);
		bool valid() const { return m_fd != INVALID; }

	private:
		int m_fd = INVALID;
		struct ev_loop *m_loop = nullptr;
		ev_io m_recv;
		async m_waiter;

		int __insert();
		int __remove();
		static void cb_accept(struct ev_loop *loop, struct ev_io *w, int revents);
	};
}