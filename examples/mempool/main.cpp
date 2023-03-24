/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev.h>

using namespace coev;

Awaiter<int> co_mempool()
{
	co_await sleep_for(5);
	co_return 0;
}
int main()
{
	co_mempool();
	Loop::start();
	return 0;
}