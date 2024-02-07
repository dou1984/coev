#pragma once
#include <atomic>
#include <mutex>
#include "awaiter.h"
#include "async.h"

namespace coev
{
	class comutex final : public async<evlts>
	{
		std::atomic_int m_flag{0};

	public:
		awaiter lock();
		awaiter unlock();
	};
}