/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <bitset>
#include "session.h"

namespace coev::http
{
	int session::on_message_begin(http_parser *_)
	{
		auto _this = static_cast<session *>(_);
		_this->clear();
		return 0;
	}
	int session::on_headers_complete(http_parser *_)
	{
		auto _this = static_cast<session *>(_);
		_this->clear();
		LOG_CORE("header end:%s\n", _this->m_value.data());
		_this->m_header_waiter.resume();
		return 0;
	}
	int session::on_message_complete(http_parser *_)
	{
		auto _this = static_cast<session *>(_);
		_this->m_finish_waiter.resume_next_loop();
		_this->clear();
		LOG_CORE("message end\n");
		return 0;
	}
	int session::on_url(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<session *>(_);
		_this->m_value = std::string_view(at, length);
		_this->m_url_waiter.resume();
		LOG_CORE("url:%s\n", _this->m_value.data());
		return 0;
	}
	int session::on_status(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<session *>(_);
		_this->m_value = std::string_view(at, length);
		_this->m_status_waiter.resume();
		LOG_CORE("status:%s\n", _this->m_value.data());
		return 0;
	}
	int session::on_header_field(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<session *>(_);
		_this->clear();
		_this->m_key = std::string_view(at, length);
		return 0;
	}
	int session::on_header_value(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<session *>(_);
		_this->m_value = std::string_view(at, length);
		_this->m_header_waiter.resume();
		LOG_CORE("header:%s:%s\n", _this->m_key.data(), _this->m_value.data());
		return 0;
	}
	int session::on_body(http_parser *_, const char *at, size_t length)
	{
		auto _this = static_cast<session *>(_);
		_this->m_value = std::string_view(at, length);
		_this->m_body_waiter.resume();
		LOG_CORE("message:%s\n", _this->m_value.data());
		return 0;
	}
	http_parser_settings session::m_settings = {
		.on_message_begin = session::on_message_begin,
		.on_url = session::on_url,
		.on_status = session::on_status,
		.on_header_field = session::on_header_field,
		.on_header_value = session::on_header_value,
		.on_headers_complete = session::on_headers_complete,
		.on_body = session::on_body,
		.on_message_complete = session::on_message_complete,
	};

	session::session()
	{
		http_parser_init(this, HTTP_REQUEST);
	}
	int session::parse(const char *buffer, int size)
	{
		return http_parser_execute(this, &m_settings, buffer, size);
	}
	void session::clear()
	{
		m_key = "";
		m_value = "";
	}
	awaitable<int> session::get_request(io_context &io, request &request)
	{
		co_task _task;
		auto _finished = _task << finished();
		auto _timeout = _task << sleep_for(m_timeout);
		_task << get_url(request.url);
		_task << get_headers(request.headers);
		_task << get_body(request.body);
		_task << get_status(request.status);
		co_start << parse(io);
		do
		{
			auto r = co_await _task.wait();
			LOG_CORE("co_await result: %ld\n", r);
			if (r == _timeout)
			{
				co_return INVALID;
			}
			if (r == _finished)
			{
				LOG_CORE("finished\n")
				co_return 0;
			}
		} while (true);
	}
	awaitable<void> session::get_url(std::string &url)
	{
		co_await m_url_waiter.suspend();
		url = m_value;
	}
	awaitable<void> session::get_headers(std::unordered_map<std::string, std::string> &headers)
	{
		while (true)
		{
			co_await m_header_waiter.suspend();
			if (m_key.size() == 0 && m_value.size() == 0)
			{
				break;
			}
			headers.emplace(m_key, m_value);
		}
	}
	awaitable<void> session::get_body(std::string &body)
	{
		co_await m_body_waiter.suspend();
		body = m_value;
	}
	awaitable<void> session::get_status(std::string &status)
	{
		co_await m_status_waiter.suspend();
		status = m_value;
	}
	awaitable<void> session::finished()
	{
		co_await m_finish_waiter.suspend();
	}
	awaitable<void> session::parse(io_context &io)
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