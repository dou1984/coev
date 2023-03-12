#include <thread>
#include <atomic>
#include <chrono>
#include <coev.h>

using namespace coev;

int g_total = 0;
Mutex g_mutex;

Awaiter<int> test_go()
{
	auto now = std::chrono::system_clock::now();
	for (int i = 0; i < 100000; i++)
	{
		TRACE();
		co_await g_mutex.lock();
		TRACE();
		g_total += 1;
		co_await g_mutex.unlock();
		TRACE();
	}
	auto r = std::chrono::system_clock::now() - now;
	printf("%d %ld\n", g_total, r.count());
	co_return 0;
}

int main()
{
	Routine r;
	for (int i = 0; i < 8; i++)
	{
		r.add(test_go);
	}
	r.join();

	return 0;
}