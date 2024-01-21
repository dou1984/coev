#pragma once
#include <atomic>
#include <mutex>
#include "awaiter.h"
#include "eventchain.h"

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