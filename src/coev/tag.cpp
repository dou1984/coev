#include <atomic>
#include "threadlocal.h"
#include "tag.h"

namespace coev
{
	static std::atomic<uint64_t> g_index{0};
	uint64_t ttag()
	{
		thread_local uint64_t _tag{g_index++};
		return _tag;
	}
}