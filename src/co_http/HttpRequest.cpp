/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "HttpRequest.h"

namespace coev
{
	HttpRequest::request::request(request &&o)
	{
		*this = std::move(o);
	}
	const HttpRequest::request &HttpRequest::request::operator=(request &&o)
	{
		url = std::move(o.url);
		headers = std::move(o.headers);
		body = std::move(o.body);
		status = std::move(o.status);
		return *this;
	}

	int HttpRequest::on_message_begin(http_parser *_)
	{
		auto _this = static_cast<HttpRequest *>(_);
		_this->clear();
		return 0;
	}
	int HttpRequest::on_headers_complete(http_parser *_)
	{
		auto _this = static_cast<HttpRequest *>(_);
		_this->clear();
		LOG_CORE("<header end>%s\n", _this->m_value.data());
		_this->m_header_waiter.resume();
		return 0;
	}
	int HttpRequest::on_message_complete(http_parser *_)
	{
		auto _this = static_cast<HttpRequest *>(_);
		_this->m_finish_waiter.resume_next_loop();
		_this->clear();
		LOG_CORE("<message end>%s\n", _this->m_value.data());
		return 0;
	}
	int HttpRequest::on_url(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<HttpRequest *>(_);
		_this->m_value = std::string_view(at, length);
		_this->m_url_waiter.resume();
		LOG_CORE("<url>%s\n", _this->m_value.data());
		return 0;
	}
	int HttpRequest::on_status(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<HttpRequest *>(_);
		_this->m_value = std::string_view(at, length);
		_this->m_status_waiter.resume();
		LOG_CORE("<status>%s\n", _this->m_value.data());
		return 0;
	}
	int HttpRequest::on_header_field(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<HttpRequest *>(_);
		_this->clear();
		_this->m_key = std::string_view(at, length);
		return 0;
	}
	int HttpRequest::on_header_value(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<HttpRequest *>(_);
		_this->m_value = std::string_view(at, length);
		_this->m_header_waiter.resume();
		LOG_CORE("<header>:%s:%s\n", _this->m_key.data(), _this->m_value.data());
		return 0;
	}
	int HttpRequest::on_body(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<HttpRequest *>(_);
		_this->m_value = std::string_view(at, length);
		_this->m_body_waiter.resume();
		LOG_CORE("<message>%s\n", _this->m_value.data());
		return 0;
	}
	http_parser_settings HttpRequest::m_settings = {
		.on_message_begin = HttpRequest::on_message_begin,
		.on_url = HttpRequest::on_url,
		.on_status = HttpRequest::on_status,
		.on_header_field = HttpRequest::on_header_field,
		.on_header_value = HttpRequest::on_header_value,
		.on_headers_complete = HttpRequest::on_headers_complete,
		.on_body = HttpRequest::on_body,
		.on_message_complete = HttpRequest::on_message_complete,
	};

	HttpRequest::HttpRequest()
	{
		http_parser_init(this, HTTP_REQUEST);
	}
	int HttpRequest::parse(const char *buffer, int size)
	{
		return http_parser_execute(this, &m_settings, buffer, size);
	}
	void HttpRequest::clear()
	{
		m_key = "";
		m_value = "";
	}
	awaitable<int, HttpRequest::request> HttpRequest::get_request(io_context &io)
	{
		co_task _task;
		_task << finished();
		_task << sleep_for(m_timeout);
		_task << get_url();
		_task << get_headers();
		_task << get_body();
		_task << parse(io);
		int r = 0;
		while ((r = co_await _task.wait()) >= 2)
		{
			LOG_CORE("result: %d\n", r);
		}
		co_return {r, std::move(m_request)};
	}
	awaitable<void> HttpRequest::get_url()
	{
		co_await m_url_waiter.suspend();
		m_request.url = m_value;
	}
	awaitable<void> HttpRequest::get_headers()
	{
		while (true)
		{
			co_await m_header_waiter.suspend();
			if (m_key.size() == 0 && m_value.size() == 0)
			{
				break;
			}
			m_request.headers.emplace(m_key, m_value);
		}
	}
	awaitable<void> HttpRequest::get_body()
	{
		co_await m_body_waiter.suspend();
		m_request.body = m_value;
	}
	awaitable<void> HttpRequest::get_status()
	{
		co_await m_status_waiter.suspend();
		m_request.status = m_value;
	}
	awaitable<void> HttpRequest::finished()
	{
		co_await m_finish_waiter.suspend();
	}
	awaitable<void> HttpRequest::parse(io_context &io)
	{
		while (io)
		{
			char buffer[0x1000];
			auto r = co_await io.recv(buffer, sizeof(buffer));
			if (r == INVALID)
			{
				break;
			}
			r = parse(buffer, r);
			if (r == 0)
			{
				break;
			}
		}
	}
}