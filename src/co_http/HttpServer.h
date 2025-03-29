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
#include "HttpRequest.h"

namespace coev
{
	class HttpServer final : public tcp::server
	{
	public:
		HttpServer() = default;
		virtual ~HttpServer() = default;

	private:
		int m_timeout = 15;
	};

}