/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <algorithm>
#include "loop.h"
#include "serverpool.h"

namespace coev::tcp
{
	server &serverpool::get()
	{
		auto _tag = ttag();
		std::lock_guard<std::mutex> _(m_mutex);
		auto &s = m_pool[_tag];
		if (s.m_fd == INVALID)
		{
			s.m_fd = m_fd;
			s.__insert(_tag);
		}
		return s;
	}
	int serverpool::start(const char *ip, int size)
	{
		auto _tag = ttag();
		std::lock_guard<std::mutex> _(m_mutex);
		auto &s = m_pool[_tag];
		if (m_fd == INVALID)
		{
			m_fd = s.start(ip, size);
			s.__insert(_tag);
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
				s.m_fd = INVALID;
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