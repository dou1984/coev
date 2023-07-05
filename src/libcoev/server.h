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

namespace coev::tcp
{
	struct server final : public EVRecv
	{
		server() = default;
		virtual ~server();
		int start(const char *ip, int port);
		int stop();
		operator bool() const;

		int m_fd = INVALID;
		ev_io m_Reav;
		int __insert(uint32_t _tag);
		int __remove(uint32_t _tag);
		static void cb_accept(struct ev_loop *loop, struct ev_io *w, int revents);
	};
}