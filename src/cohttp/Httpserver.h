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
#include "Httprequest.h"

namespace coev
{
	class Httpserver final
	{
	public:
		Httpserver(const char *, int);
		virtual ~Httpserver() = default;

		awaitable<int> accept(host &);
		auto &get() { return m_pool.get(); }
		
	private:
		tcp::serverpool m_pool;
		int m_timeout = 15;
	};

}