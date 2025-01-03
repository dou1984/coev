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

namespace coev
{
	class Httpparser : http_parser
	{
	public:
		Httpparser();
		virtual ~Httpparser() = default;
		int parse(const char *, int);

		awaitable<std::string> get_url();
		awaitable<std::string, std::string> get_header();
		awaitable<std::string> get_body();

	private:
		std::string m_key;
		std::string m_value;
		async m_url_listener;
		async m_body_listener;
		async m_header_listener;

		static http_parser_settings m_settings;
		void clear();
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