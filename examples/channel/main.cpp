/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <thread>
#include <atomic>
#include <coev.h>

using namespace coev;

channel<int> ch;

std::atomic<int> total = 0;
awaiter<int> go()
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

	routine r;
	for (int i = 0; i < 8; i++)
		r.add(go);
	r.join();
	return 0;
}