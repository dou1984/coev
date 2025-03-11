/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <unordered_set>
#include <algorithm>
#include <mutex>
#include "gtid.h"

namespace coev
{

#define TID_MAX 1000000000
	struct unique_id
	{
		static std::unordered_set<uint64_t> m_tidset;
		static std::mutex m_mutex;
		static uint64_t m_next_id;
		uint64_t m_id = 0;
		unique_id()
		{
			std::lock_guard<std::mutex> _(m_mutex);
			do
			{
				if (m_next_id > TID_MAX)
				{
					m_next_id = 0;
				}
				m_id = m_next_id++;

			} while (std::find(m_tidset.begin(), m_tidset.end(), m_id) != m_tidset.end());
			m_tidset.insert(m_id);
		}
		~unique_id()
		{
			std::lock_guard<std::mutex> _(m_mutex);
			m_tidset.erase(m_id);
		}
	};
	uint64_t unique_id::m_next_id = 0;
	std::unordered_set<uint64_t> unique_id::m_tidset;
	std::mutex unique_id::m_mutex;
	uint64_t gtid()
	{
		thread_local unique_id _;
		return _.m_id;
	}
	
}