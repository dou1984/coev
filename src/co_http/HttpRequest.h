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

	class HttpRequest : http_parser
	{
	public:
		struct request
		{
			std::string url;
			std::string body;
			std::string status;
			std::unordered_map<std::string, std::string> headers;
			request() = default;
			request(request &&o);
			const request &operator=(request &&o);
		};

	public:
		HttpRequest();
		virtual ~HttpRequest() = default;
		awaitable<void> parse(io_context &io);

		awaitable<int, request> get_request(io_context &io);
		awaitable<void> get_url();
		awaitable<void> get_headers();

		awaitable<void> get_body();
		awaitable<void> get_status();
		awaitable<void> finished();

	private:
		const int m_timeout = 15;
		request m_request;

	private:
		int parse(const char *, int);
		std::string_view m_key;
		std::string_view m_value;
		async m_url_waiter;
		async m_body_waiter;
		async m_header_waiter;
		async m_status_waiter;
		async m_finish_waiter;

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