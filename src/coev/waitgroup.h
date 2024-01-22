#pragma once
#include <atomic>
#include "evlist.h"
#include "awaiter.h"

namespace coev
{
	class waitgroup final : public async_mutex
	{
		std::atomic_int m_count{0};

	public:
		int add(int c = 1);
		int done();
		awaiter wait();
	};
}