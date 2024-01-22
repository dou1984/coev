#pragma once
#include <atomic>
#include <mutex>
#include "awaiter.h"
#include "evlist.h"

namespace coev
{
	class comutex final : public async_mutex
	{
		std::atomic_int m_flag{0};

	public:
		awaiter lock();
		awaiter unlock();
	};
}