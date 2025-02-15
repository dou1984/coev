/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <string>
#include <string_view>
#include <tuple>
#include <http_parser.h>
#include <coev/coev.h>
#include <cosys/cosys.h>

namespace coev
{
	class Httprequest : http_parser
	{
	public:
		Httprequest();
		virtual ~Httprequest() = default;
		awaitable<int> parse(iocontext &io);

		awaitable<std::string_view> get_url();
		awaitable<bool, std::string_view, std::string_view> get_header();
		awaitable<std::string_view> get_body();
		awaitable<std::string_view> get_status();

	private:
		int parse(const char *, int);
		std::string_view m_key;
		std::string_view m_value;
		async m_url_listener;
		async m_body_listener;
		async m_header_listener;
		async m_status_listener;

		static http_parser_settings m_settings;
		void clear();

	private:
		static int on_message_begin(http_parser *_);
		static int on_headers_complete(http_parser *_);
		static int on_message_complete(http_parser *_);
		static int on_url(http_parser *_, const char *, size_t);
		static int on_status(http_parser *_, const char *, size_t);
		static int on_header_field(http_parser *_, const char *, size_t);
		static int on_header_value(http_parser *_, const char *, size_t);
		static int on_body(http_parser *_, const char *, size_t);
	};
}