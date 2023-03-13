/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <http_parser.h>
#include "../coev.h"

namespace coev
{
	struct Httprequest : http_parser,  EVRecv
	{
		Httprequest();
		virtual ~Httprequest() = default;
		int parse(const char*, int);
		static int on_message_begin(http_parser *_);
		static int on_headers_complete(http_parser *_);
		static int on_message_complete(http_parser *_);
		static int on_url(http_parser *_, const char *at, size_t length);
		static int on_header_field(http_parser *_, const char *at, size_t length);
		static int on_header_value(http_parser *_, const char *at, size_t length);
		static int on_body(http_parser *_, const char *at, size_t length);		
	};
}