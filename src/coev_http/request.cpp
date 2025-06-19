#include <coev/coev.h>
#include "request.h"

namespace coev::http
{
	request::request(request &&o)
	{
		*this = std::move(o);
	}
	const request &request::operator=(request &&o)
	{
		url = std::move(o.url);
		headers = std::move(o.headers);
		body = std::move(o.body);
		status = std::move(o.status);
		return *this;
	}
	request::~request()
	{
		LOG_CORE("release %p\n", this);
	}
}