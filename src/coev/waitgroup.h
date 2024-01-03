#pragma once
#include <atomic>
#include "eventchainmutex.h"
#include "awaiter.h"

namespace coev
{
	class waitgroup final : public EVMutex
	{
	public:
		int add(int c = 1);
		int done();
		event wait();

	private:
		std::atomic_int m_count{0};
	};
}