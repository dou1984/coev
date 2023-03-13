#pragma once
#include "Loop.h"
#include "IOContext.h"

namespace coev
{
	struct Udp final
	{
		static IOContext bind(const char *ip, int port);
		static IOContext socket();
	};
}