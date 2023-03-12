#pragma once
#include "Loop.h"
#include "Client.h"

namespace coev
{
	struct Udp final
	{
		static Client bind(const char *ip, int port);
		static Client socket();
	};
}