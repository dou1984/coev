#include "Loop.h"
#include "ServerPool.h"

namespace coev
{
	ServerPool::operator Server &()
	{
		auto _tag = Loop::tag();
		if (_tag < m_pool.size())
		{
			std::lock_guard<std::mutex> _(m_mutex);
			auto &s = m_pool[_tag];
			if (s.m_fd == INVALID)
			{
				s.m_fd = m_fd;
				s.__insert(_tag);
			}
			return s;
		}
		throw(" server pool error");
	}
	int ServerPool::start(const char *ip, int size)
	{
		auto _tag = Loop::tag();
		if (_tag < m_pool.size())
		{
			auto &s = m_pool[_tag];
			if (m_fd == INVALID)
			{
				std::lock_guard<std::mutex> _(m_mutex);
				m_fd = s.start(ip, size);
			}
			return m_fd;
		}
		return 0;
	}
	int ServerPool::stop()
	{
		for (size_t i = 0; i < m_pool.size(); i++)
		{
			std::lock_guard<std::mutex> _(m_mutex);
			auto &s = m_pool[i];
			if (s.m_fd != INVALID)
			{
				s.__remove(i);
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