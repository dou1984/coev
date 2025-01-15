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
	int Httprequest::on_message_begin(http_parser *_)
	{
		auto _this = static_cast<Httprequest *>(_);
		_this->clear();
		return 0;
	}
	int Httprequest::on_headers_complete(http_parser *_)
	{
		auto _this = static_cast<Httprequest *>(_);
		_this->clear();
		_this->m_header_listener.resume();
		return 0;
	}
	int Httprequest::on_message_complete(http_parser *_)
	{
		auto _this = static_cast<Httprequest *>(_);
		_this->m_body_listener.resume();
		_this->clear();
		return 0;
	}
	int Httprequest::on_url(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httprequest *>(_);
		_this->m_value = std::string_view(at, length);
		LOG_CORE("<url>%s\n", _this->m_value.data());
		_this->m_url_listener.resume();
		return 0;
	}
	int Httprequest::on_status(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httprequest *>(_);
		_this->m_value = std::string_view(at, length);
		LOG_CORE("<status>%s\n", _this->m_value.data());
		_this->m_status_listener.resume();
		return 0;
	}
	int Httprequest::on_header_field(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httprequest *>(_);
		_this->m_key = std::string_view(at, length);
		return 0;
	}
	int Httprequest::on_header_value(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httprequest *>(_);
		_this->m_value = std::string_view(at, length);
		LOG_CORE("<header>%s:%s\n", _this->m_key.data(), _this->m_value.data());
		_this->m_header_listener.resume();
		return 0;
	}
	int Httprequest::on_body(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<Httprequest *>(_);
		_this->m_value = std::string_view(at, length);
		LOG_CORE("<message>%s\n", _this->m_value.data());
		_this->m_body_listener.resume();
		return 0;
	}
	http_parser_settings Httprequest::m_settings = {
		.on_message_begin = Httprequest::on_message_begin,
		.on_url = Httprequest::on_url,
		.on_status = Httprequest::on_status,
		.on_header_field = Httprequest::on_header_field,
		.on_header_value = Httprequest::on_header_value,
		.on_headers_complete = Httprequest::on_headers_complete,
		.on_body = Httprequest::on_body,
		.on_message_complete = Httprequest::on_message_complete,
	};
	Httprequest::Httprequest()
	{
		http_parser_init(this, HTTP_REQUEST);
	}
	int Httprequest::parse(const char *buffer, int size)
	{
		return http_parser_execute(this, &m_settings, buffer, size);
	}
	void Httprequest::clear()
	{
		m_key = std::string_view();
		m_value = std::string_view();
	}
	awaitable<std::string_view> Httprequest::get_url()
	{
		co_await m_url_listener.suspend();
		co_return std::move(m_value);
	}
	awaitable<bool, std::string_view, std::string_view> Httprequest::get_header()
	{
		co_await m_header_listener.suspend();
		if (m_key.size() == 0 && m_value.size() == 0)
		{
			co_return {false, m_key, m_value};
		}
		co_return {true, std::move(m_key), std::move(m_value)};
	}
	awaitable<std::string_view> Httprequest::get_body()
	{
		co_await m_body_listener.suspend();
		co_return std::move(m_value);
	}
	awaitable<std::string_view> Httprequest::get_status()
	{
		co_await m_status_listener.suspend();
		co_return std::move(m_value);
	}
	awaitable<int> Httprequest::parse(iocontext &io)
	{
		while (io)
		{
			char buffer[0x1000];
			auto r = co_await io.recv(buffer, sizeof(buffer));
			if (r == INVALID)
			{
				co_return INVALID;
			}
			parse(buffer, r);
		}
		co_return 0;
	}
}