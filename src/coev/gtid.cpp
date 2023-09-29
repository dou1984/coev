#include <atomic>
#include <unordered_set>
#include <mutex>
#include "threadlocal.h"
#include "gtid.h"

namespace coev
{
	static uint64_t g_index{0};
	static std::unordered_set<uint64_t> all_index;
	static std::mutex g_mutex;
	struct threadid
	{
		uint64_t m_tid = 0;
		threadid()
		{
			std::lock_guard<std::mutex> _(g_mutex);
			while (all_index.find(g_index) != all_index.end())
			{
				g_index++;
			}
			m_tid = g_index++;
			all_index.emplace(m_tid);
		}
		~threadid()
		{
			std::lock_guard<std::mutex> _(g_mutex);
			all_index.erase(m_tid);
		}
	};
	uint64_t gtid()
	{
		thread_local threadid _tid;
		return _tid.m_tid;
	}
}