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

namespace coev
{
	struct iocontext : EVRecv, EVSend
	{
		int m_fd = INVALID;
		uint32_t m_tag = 0;
		ev_io m_Read;
		ev_io m_Write;

		iocontext() = default;
		iocontext(int fd);
		iocontext(iocontext &&) = delete;
		virtual ~iocontext();
		int close();
		operator bool() const;
		const iocontext &operator=(iocontext &&);

		int __finally();
		int __init();
		static void cb_write(struct ev_loop *loop, struct ev_io *w, int revents);
		static void cb_read(struct ev_loop *loop, struct ev_io *w, int revents);
	};

}
