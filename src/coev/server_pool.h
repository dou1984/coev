/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <mutex>
#include <unordered_map>
#include "server.h"

namespace coev
{
	template <class T>
	class server_pool
	{
		int m_fd = INVALID;
		std::unordered_map<uint64_t, T> m_pool;
		std::mutex m_mutex;

	public:
		T &get()
		{
			auto _tid = gtid();
			std::lock_guard<std::mutex> _(m_mutex);
			auto &srv = m_pool[_tid];
			if (m_fd == INVALID)
			{
				LOG_ERR("server is not running");
			}
			else if (!srv.valid())
			{
				srv.bind(m_fd);
			}
			return srv;
		}
		int start(const char *ip, int port)
		{
			auto _tid = gtid();
			std::lock_guard<std::mutex> _(m_mutex);
			auto &srv = m_pool[_tid];
			if (m_fd == INVALID)
			{
				m_fd = srv.start(ip, port);
				LOG_CORE("server_pool start fd=%d ip=%s port=%d", m_fd, ip, port);
			}
			return m_fd;
		}
		int stop()
		{
			std::lock_guard<std::mutex> _(m_mutex);
			for (auto it = m_pool.begin(); it != m_pool.end(); ++it)
			{
				auto &srv = it->second;
				if (srv.valid())
				{
					srv.__remove();
				}
			}
			if (m_fd != INVALID)
			{
				::close(m_fd);
				m_fd = INVALID;
			}
			return 0;
		}
	};

}