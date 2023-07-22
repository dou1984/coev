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
awaiter go()
{
	int x = 0;
	for (int i = 0; i < 100000; i++)
	{
		x++;
		co_await ch.set(std::move(x));
		co_await ch.get(x);
	}
	total += x;
	x = total;
	printf("%d\n", x);
	co_return x;
}
int main()
{

	routine::instance().add(8, go);
	routine::instance().join();
	return 0;
}