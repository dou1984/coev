/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <ev.h>
#include <coev/coev.h>
#include "socket.h"
#include "io_context.h"

namespace coev
{
	class client : public io_context
	{
	public:
		client();
		virtual ~client();
		awaitable<int> connect(const char *, int);

	private:
		void __initial() ;
		int __insert();
		int __remove();
		int __connect(const char *ip, int port);
		int __connect(int fd, const char *ip, int port);
		static void cb_connect(struct ev_loop *loop, struct ev_io *w, int revents);
	};

}
