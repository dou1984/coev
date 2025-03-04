/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <thread>
#include <atomic>
#include <coev/coev.h>
#include <cosys/cosys.h>

using namespace coev;

co_channel<int> ch;

std::atomic<int> total = 0;
awaitable<int> go()
{
	int x = 0;
	for (int i = 0; i < 100000; i++)
	{
		x++;
		ch.set(x);
		x = co_await ch.move();
	}
	total += x;
	printf("%d\n", total.load());
	co_return x;
}
int main()
{

	running::instance().add(8, go).join();
	return 0;
}