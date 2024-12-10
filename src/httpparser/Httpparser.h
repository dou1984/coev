/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <http_parser.h>
#include <coev/coev.h>

namespace coev
{
	struct Httpparser : http_parser
	{
		Httpparser();
		virtual ~Httpparser() = default;
		int parse(const char *, int);

		std::unordered_map<std::string, std::string> m_header;
		std::string last_header;
		std::string m_body;
		std::string m_url;
		std::string m_response;
		async m_trigger;
	};
}