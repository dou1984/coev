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
#include "session.h"

namespace coev::http
{
	class server final : public tcp::server
	{
	public:
		server() = default;
		virtual ~server() = default;

	private:
		int m_timeout = 15;
	};

}