/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "Httprequest.h"

namespace coev
{
	int on_message_begin(http_parser *_)
	{
		return 0;
	}
	int on_headers_complete(http_parser *_)
	{
		return 0;
	}
	int on_message_complete(http_parser *_)
	{
		auto _this = static_cast<Httprequest *>(_);
		_this->EVRecv::resume();
		return 0;
	}
	int on_url(http_parser *_, const char *at, size_t length)
	{
		return 0;
	}
	int on_header_field(http_parser *_, const char *at, size_t length)
	{
		return 0;
	}
	int on_header_value(http_parser *_, const char *at, size_t length)
	{
		return 0;
	}
	int on_body(http_parser *_, const char *at, size_t length)
	{
		return 0;
	}
	static http_parser_settings g_settings = {
		.on_message_begin = on_message_begin,
		.on_url = on_url,
		.on_status = 0,
		.on_header_field = on_header_field,
		.on_header_value = on_header_value,
		.on_headers_complete = on_headers_complete,
		.on_body = on_body,
		.on_message_complete = on_message_complete,
	};
	Httprequest::Httprequest()
	{
		http_parser_init(this, HTTP_REQUEST);
	}
	int Httprequest::parse(const char *buffer, int size)
	{
		return http_parser_execute(this, &g_settings, buffer, size);
	}

}