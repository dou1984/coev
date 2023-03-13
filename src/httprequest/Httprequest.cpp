#include "Httprequest.h"

namespace coev
{
	static http_parser_settings g_settings = {
		.on_message_begin = Httprequest::on_message_begin,
		.on_url = Httprequest::on_url,
		.on_status = 0,		
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
		return http_parser_execute(this, &g_settings, buffer, size);
	}
	int Httprequest::on_message_begin(http_parser *_)
	{
		return 0;
	}
	int Httprequest::on_headers_complete(http_parser *_)
	{
		return 0;
	}
	int Httprequest::on_message_complete(http_parser *_)
	{
		auto _this = static_cast<Httprequest *>(_);
		_this->EVRecv::resume_ex();
		return 0;
	}
	int Httprequest::on_url(http_parser *_, const char *at, size_t length)
	{
		return 0;
	}
	int Httprequest::on_header_field(http_parser *_, const char *at, size_t length)
	{
		return 0;
	}
	int Httprequest::on_header_value(http_parser *_, const char *at, size_t length)
	{
		return 0;
	}
	int Httprequest::on_body(http_parser *_, const char *at, size_t length)
	{
		return 0;
	}
}