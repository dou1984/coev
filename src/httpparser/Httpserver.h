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
#include <coev/coev.h>
#include <cosys/cosys.h>
#include "Httpparser.h"

namespace coev
{
	class Httpserver final
	{
	public:
		using frouter = std::function<awaitable<int>(iocontext &io, Httpparser &req)>;
		Httpserver(const char *, int, int);
		virtual ~Httpserver() = default;

		void add_router(const std::string &, const frouter &);

	private:
		tcp::serverpool m_pool;
		int m_timeout = 15;
		std::unordered_map<std::string, frouter> m_router;
		awaitable<int> dispatch(const host &addr, iocontext &io);
		awaitable<int> router(iocontext &c, Httpparser &req);
		awaitable<int> timeout(const host &addr, iocontext &io);
	};

}