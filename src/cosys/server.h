/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <ev.h>
#include "socket.h"
#include "iocontext.h"

namespace coev::tcp
{
	class server
	{
	public:
		server();
		virtual ~server();
		int start(const char *ip, int port);
		int stop();
		awaitable<int> accept(host &);
		operator bool() const { return __valid(); }

	private:
		friend class serverpool;
		int m_fd = INVALID;
		struct ev_loop *m_loop = nullptr;
		ev_io m_reav;
		async m_waiter;

		int __insert(uint64_t _tid);
		int __remove(uint64_t _tid);
		bool __valid() const;
		static void cb_accept(struct ev_loop *loop, struct ev_io *w, int revents);
	};
}