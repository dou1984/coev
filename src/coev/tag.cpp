#include <atomic>
#include <unordered_set>
#include <mutex>
#include "threadlocal.h"
#include "tag.h"

namespace coev
{
	static uint64_t g_index{0};
	static std::unordered_set<uint64_t> all_index;
	static std::mutex g_mutex;
	struct thdtag
	{
		uint64_t m_tag = 0;
		thdtag()
		{
			std::lock_guard<std::mutex> _(g_mutex);
			while (all_index.find(g_index) != all_index.end())
			{
				g_index++;
			}
			m_tag = g_index++;
			all_index.emplace(m_tag);
		}
		~thdtag()
		{
			std::lock_guard<std::mutex> _(g_mutex);
			all_index.erase(m_tag);
		}
	};
	uint64_t ttag()
	{
		thread_local thdtag _tag;
		return _tag.m_tag;
	}
}