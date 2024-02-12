/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <unordered_map>
#include <functional>
#include "../coloop.h"
#include "Httpparser.h"

namespace coev
{
	class Httpserver final
	{
	public:
		using frouter = std::function<awaiter(iocontext &io, Httpparser &req)>;
		Httpserver(const char *, int, int);
		virtual ~Httpserver() = default;

		void add_router(const std::string &, const frouter &);

	private:
		tcp::serverpool m_pool;
		int m_timeout = 15;
		std::unordered_map<std::string, frouter> m_router;
		awaiter dispatch(const ipaddress &addr, iocontext &io);
		awaiter router(iocontext &c, Httpparser &req);
		awaiter timeout(const ipaddress &addr, iocontext &io);
	};

}