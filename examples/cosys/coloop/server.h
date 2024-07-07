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

	class server final 
	{
	public:
		using faccept = std::function<awaiter(const ipaddress &, iocontext &)>;
		server() = default;
		virtual ~server();
		int start(const char *ip, int port);
		int stop();
		awaiter accept(const faccept &dispatch);

	private:
		friend class serverpool;
		int m_fd = INVALID;
		ev_io m_Reav;
		faccept m_dispatch;
		async m_trigger;

		int __insert(uint64_t _tid);
		int __remove(uint64_t _tid);
		bool __valid() const;
		awaiter __accept();
		static void cb_accept(struct ev_loop *loop, struct ev_io *w, int revents);
	};
}