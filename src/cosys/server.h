/*
 *	cosys - c++20 coroutine library
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
		using faccept = std::function<awaitable<int>(const ipaddress &, iocontext &)>;
		server() = default;
		virtual ~server();
		int start(const char *ip, int port);
		int stop();
		awaitable<int> accept(const faccept &dispatch);

	private:
		friend class serverpool;
		int m_fd = INVALID;
		ev_io m_reav;
		faccept m_dispatch;
		async m_listener;

		int __insert(uint64_t _tid);
		int __remove(uint64_t _tid);
		bool __valid() const;
		awaitable<int> __accept();
		static void cb_accept(struct ev_loop *loop, struct ev_io *w, int revents);
	};
}