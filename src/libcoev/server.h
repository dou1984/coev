/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <ev.h>
#include "Socket.h"
#include "event.h"
#include "eventchain.h"
#include "awaiter.h"
#include "iocontext.h"

namespace coev::tcp
{

	struct server final : public EVRecv
	{
		using fnaccept = std::function<awaiter(const ipaddress &, iocontext &)>;
		server() = default;
		virtual ~server();
		int start(const char *ip, int port);
		int stop();
		awaiter accept(const fnaccept &dispatch);

		int m_fd = INVALID;
		ev_io m_Reav;

		int __insert(uint64_t _tag);
		int __remove(uint64_t _tag);
		bool __valid() const;
		awaiter __accept(const fnaccept &);
		static void cb_accept(struct ev_loop *loop, struct ev_io *w, int revents);
	};
}