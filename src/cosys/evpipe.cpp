#include <unordered_map>
#include <mutex>
#include "coev/coev.h"
#include "evpipe.h"

namespace coev
{
	local<evpipe> __this_pipe;
	static std::unordered_map<uint64_t, evpipe *> all_pipes;
	static std::mutex g_mutex;

	evpipe::evpipe()
	{
		m_tid = gtid();
		std::lock_guard<std::mutex> _(g_mutex);
		all_pipes.emplace(m_tid, this);
	}
	evpipe::~evpipe()
	{
		std::lock_guard<std::mutex> _(g_mutex);
		all_pipes.erase(m_tid);
	}
	void evpipe::resume(async &listener)
	{
		auto c = listener.pop_front();
		if (c == nullptr)
		{
			return;
		}
		auto ev = static_cast<event *>(c);
		if (ev->m_tid == gtid())
		{
			ev->resume();
		}
		else
		{
			std::lock_guard<std::mutex> _(g_mutex);
			if (auto it = all_pipes.find(ev->m_tid); it != all_pipes.end())
			{
				it->second->awaken::resume(ev);
			}
		}
	}
}