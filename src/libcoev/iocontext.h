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
#include "Socket.h"
#include "event.h"
#include "eventchain.h"
#include "awaiter.h"

namespace coev
{
	class iocontext : public EVRecv, public EVSend
	{
	public:
		iocontext() = default;
		iocontext(int fd);
		iocontext(iocontext &&) = delete;
		virtual ~iocontext();
		awaiter<int> send(const char *, int);
		awaiter<int> recv(char *, int);
		awaiter<int> recvfrom(char *, int, ipaddress &);
		awaiter<int> sendto(const char *, int, ipaddress &);
		awaiter<int> close();
		operator bool() const;
		const iocontext &operator=(iocontext &&);

	protected:
		uint32_t m_tag = 0;
		int m_fd = INVALID;
		ev_io m_Read;
		ev_io m_Write;

		int __finally();
		int __init();
		int __close();
		bool __valid() const;
		static void cb_write(struct ev_loop *loop, struct ev_io *w, int revents);
		static void cb_read(struct ev_loop *loop, struct ev_io *w, int revents);
	};
	using sharedIOContext = std::shared_ptr<coev::iocontext>;
}
