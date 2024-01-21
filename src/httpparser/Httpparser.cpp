/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "Httpparser.h"

namespace coev
{
	int on_message_begin(http_parser *_)
	{
		return 0;
	}
	int on_headers_complete(http_parser *_)
	{
		auto _this = static_cast<Httpparser *>(_);
		return 0;
	}
	int on_message_complete(http_parser *_)
	{
		auto _this = static_cast<Httpparser *>(_);
		resume<EVRecv>(_this);
		return 0;
	}
	int on_url(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httpparser *>(_);
		_this->m_url = std::string(at, length);
		LOG_CORE("url:%s\n", _this->m_url.c_str());
		return 0;
	}
	int on_status(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httpparser *>(_);
		LOG_CORE("status:%s\n", at);
		return 0;
	}
	int on_header_field(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httpparser *>(_);
		_this->last_header = std::string(at, length);
		LOG_CORE("header_field:%s\n", at);
		return 0;
	}
	int on_header_value(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httpparser *>(_);
		_this->m_header[_this->last_header] = std::string(at, length);
		LOG_CORE("header:%s -> %s\n", _this->last_header.c_str(), _this->m_header[_this->last_header].c_str());
		return 0;
	}
	int on_body(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httpparser *>(_);
		_this->m_body = std::string(at, length);
		LOG_CORE("body:%s\n", _this->m_body.c_str());
		return 0;
	}
	static http_parser_settings g_settings = {
		.on_message_begin = on_message_begin,
		.on_url = on_url,
		.on_status = on_status,
		.on_header_field = on_header_field,
		.on_header_value = on_header_value,
		.on_headers_complete = on_headers_complete,
		.on_body = on_body,
		.on_message_complete = on_message_complete,
	};
	Httpparser::Httpparser()
	{
		http_parser_init(this, HTTP_REQUEST);
	}
	int Httpparser::parse(const char *buffer, int size)
	{
		return http_parser_execute(this, &g_settings, buffer, size);
	}

}