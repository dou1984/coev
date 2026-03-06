/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <ev.h>
#include <memory>
#include "async.h"
#include "socket.h"

namespace coev
{
#define IO_CLI (0x1 << 0)
#define IO_SSL (0x1 << 1)
#define IO_TCP (0x1 << 2)
#define IO_UDP (0x1 << 3)

	class io_context
	{
	public:
		io_context() noexcept;
		io_context(int fd) noexcept;
		io_context(io_context &&) = delete;
		virtual ~io_context() noexcept;
		virtual awaitable<int> send(const char *, int) noexcept;
		virtual awaitable<int> recv(char *, int) noexcept;
		template <class T = std::string>
		awaitable<int> send(const T &msg) noexcept
		{
			return send(msg.data(), msg.size());
		}
		template <class T = std::string>
		awaitable<int> recv(T &msg) noexcept
		{
			return recv(msg.data(), msg.size());
		}

		virtual awaitable<int> recvfrom(char *, int, addrInfo &) noexcept;
		virtual awaitable<int> sendto(const char *, int, addrInfo &) noexcept;
		int close() noexcept;
		operator bool() const noexcept;

	protected:
		uint64_t m_tid = 0;
		struct ev_loop *m_loop = nullptr;
		int m_fd = INVALID;
		int m_type = 0;
		ev_io m_read;
		ev_io m_write;

		async m_r_waiter;
		async m_w_waiter;
		int __finally() noexcept;
		int __initial() noexcept;
		int __close() noexcept;
		bool __valid() const noexcept;
		bool __invalid() const noexcept;
		int __del_write() noexcept;
		bool __is_client() const noexcept { return m_type & IO_CLI; }
		bool __is_ssl() const noexcept { return m_type & IO_SSL; }
		static void cb_write(struct ev_loop *loop, struct ev_io *w, int revents) noexcept;
		static void cb_read(struct ev_loop *loop, struct ev_io *w, int revents) noexcept;
	};
}
