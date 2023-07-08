#include <atomic>
#include "threadlocal.h"
#include "tag.h"

namespace coev
{
	static std::atomic<uint32_t> g_index{0};
	int ttag()
	{
		thread_local uint32_t _tag{g_index++};
		return _tag;
	}
}