#pragma once
#include <array>
#include <mutex>
#include "Server.h"

namespace coev
{
	class ServerPool final
	{
		int m_fd = INVALID;
		std::array<Server, 0x40> m_pool;
		std::mutex m_mutex;

	public:
		operator Server &();
		int start(const char *, int);
		int stop();
	};

}