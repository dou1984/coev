#include <thread>
#include <atomic>
#include <coev.h>

using namespace coev;

Channel<int> ch;

std::atomic<int> total = 0;
Awaiter<int> go()
{
	int x = 0;
	for (int i = 0; i < 100000; i++)
	{
		x++;
		co_await ch.set(x);
		co_await ch.get(x);
	}
	total += x;
	x = total;
	printf("%d\n", x);
	co_return x;
}
int main()
{
	ingore_signal(SIGPIPE);

	Routine r;
	for (int i = 0; i < 8; i++)
		r.add(go);
	r.join();
	return 0;
}