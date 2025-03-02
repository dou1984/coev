/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <mutex>
#include <unordered_map>
#include "server.h"

namespace coev
{
	template <class S>
	class server_pool
	{
		int m_fd = INVALID;
		std::unordered_map<uint64_t, S> m_pool;
		std::mutex m_mutex;

	public:
		S &get()
		{
			auto _tid = gtid();
			std::lock_guard<std::mutex> _(m_mutex);
			auto &s = m_pool[_tid];
			if (m_fd == INVALID)
			{
			}
			else if (!s.valid())
			{
				s.insert(m_fd);
			}
			return s;
		}
		int start(const char *ip, int port)
		{
			auto _tid = gtid();
			std::lock_guard<std::mutex> _(m_mutex);
			auto &s = m_pool[_tid];
			if (m_fd == INVALID)
			{
				m_fd = s.start(ip, port);				
			
			}
			return m_fd;
		}
		int stop()
		{
			std::lock_guard<std::mutex> _(m_mutex);
			for (auto it = m_pool.begin(); it != m_pool.end(); ++it)
			{
				auto &s = it->second;
				if (s.m_fd != INVALID)
				{
					s.__remove();
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