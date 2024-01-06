#pragma once
#include <mutex>
#include "eventchainmutex.h"
#include "awaiter.h"

namespace coev
{
	class comutex final : public EVMutex
	{
		std::atomic_int m_flag{0};

	public:
		awaiter lock();
		awaiter unlock();
	};
}