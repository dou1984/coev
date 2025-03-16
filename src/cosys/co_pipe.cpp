#include <unordered_map>
#include <mutex>
#include "coev/coev.h"
#include "co_pipe.h"

namespace coev
{
	local<co_pipe> __this_pipe;
	static std::unordered_map<uint64_t, co_pipe *> all_pipes;
	static std::mutex g_mutex;

	co_pipe::co_pipe()
	{
		m_tid = gtid();
		std::lock_guard<std::mutex> _(g_mutex);
		all_pipes.emplace(m_tid, this);
	}
	co_pipe::~co_pipe()
	{
		std::lock_guard<std::mutex> _(g_mutex);
		all_pipes.erase(m_tid);
	}
	void co_pipe::resume(async &listener)
	{
		auto c = listener.pop_front();
		if (c == nullptr)
		{
			return;
		}
		auto ev = static_cast<co_event *>(c);
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