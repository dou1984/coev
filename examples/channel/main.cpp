/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <thread>
#include <atomic>
#include "../cosys/coloop.h"

using namespace coev;

channel<int> ch;

std::atomic<int> total = 0;
awaitable<int> go()
{
	int x = 0;
	for (int i = 0; i < 100000; i++)
	{
		x++;
		ch.set(std::move(x));
		co_await ch.get(x);
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