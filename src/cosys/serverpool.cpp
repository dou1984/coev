/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <algorithm>
#include "cosys.h"
#include "serverpool.h"

namespace coev::tcp
{
	server &serverpool::get()
	{
		auto _tid = gtid();
		std::lock_guard<std::mutex> _(m_mutex);
		auto &s = m_pool[_tid];
		if (s.m_fd == INVALID)
		{
			s.m_fd = m_fd;
			s.__insert(_tid);
		}
		return s;
	}
	int serverpool::start(const char *ip, int size)
	{
		auto _tid = gtid();
		std::lock_guard<std::mutex> _(m_mutex);
		auto &s = m_pool[_tid];
		if (m_fd == INVALID)
		{
			m_fd = s.start(ip, size);
			s.__insert(_tid);
		}
		return m_fd;
	}
	int serverpool::stop()
	{
		std::lock_guard<std::mutex> _(m_mutex);
		for (auto it = m_pool.begin(); it != m_pool.end(); ++it)
		{
			auto &s = it->second;
			if (s.m_fd != INVALID)
			{
				s.__remove(it->first);
			}
		}
		if (m_fd != INVALID)
		{
			::close(m_fd);
			m_fd = INVALID;
		}
		return 0;
	}

}