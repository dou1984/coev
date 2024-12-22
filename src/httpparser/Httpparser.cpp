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
		auto _this = static_cast<Httpparser *>(_);
		_this->clear();
		return 0;
	}
	int on_headers_complete(http_parser *_)
	{
		auto _this = static_cast<Httpparser *>(_);
		_this->clear();
		_this->m_header_listener.resume();
		return 0;
	}
	int on_message_complete(http_parser *_)
	{
		auto _this = static_cast<Httpparser *>(_);
		_this->m_body_listener.resume();
		_this->clear();
		return 0;
	}
	int on_url(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httpparser *>(_);
		_this->m_value = std::string(at, length);
		LOG_CORE("<url>%s\n", _this->m_value.data());
		_this->m_url_listener.resume();
		return 0;
	}
	int on_status(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httpparser *>(_);
		_this->m_value = std::string(at, length);
		LOG_CORE("<status>%s\n", _this->m_value.data());
		return 0;
	}
	int on_header_field(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httpparser *>(_);
		_this->m_key = std::string(at, length);
		return 0;
	}
	int on_header_value(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httpparser *>(_);
		_this->m_value = std::string(at, length);
		LOG_CORE("<header>%s:%s\n", _this->m_key.data(), _this->m_value.data());
		_this->m_header_listener.resume();
		return 0;
	}
	int on_body(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httpparser *>(_);
		_this->m_value = std::string(at, length);
		LOG_CORE("<message>%s\n", _this->m_value.data());
		_this->m_body_listener.resume();
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
	void Httpparser::clear()
	{
		m_key.clear();
		m_value.clear();
	}
	awaitable<std::string> Httpparser::get_url()
	{
		co_await m_url_listener.suspend();
		co_return m_value;
	}
	awaitable<std::string, std::string> Httpparser::get_header()
	{
		co_await m_header_listener.suspend();
		co_return m_key, m_value;
	}
	awaitable<std::string> Httpparser::get_body()
	{
		co_await m_body_listener.suspend();
		co_return m_value;
	}
}