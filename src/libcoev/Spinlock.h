#pragma once
#include <atomic>
#include <cstddef>

namespace coev
{
	struct Spinlock
	{
		const int on = 1;
		const int off = 0;
		std::atomic<int> flag{off};

		bool try_lock();
		void lock(int t = 0);
		void unlock();
	};
}