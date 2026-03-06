/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <unordered_map>
#include <functional>
#include <coev/coev.h>
#include "session.h"

namespace coev::http
{
	class server final : public tcp::server
	{
	public:
		using route = std::function<awaitable<void>(io_context &, request &)>;
		server() = default;
		virtual ~server() = default;
		void set_router(const std::string &path, const route &);

		awaitable<void> worker();

	private:
		awaitable<void> __handle(int fd, addrInfo &h);

	private:
		int m_timeout = 15;
		co_task m_task;
		std::map<std::string, route> m_router;
	};

}