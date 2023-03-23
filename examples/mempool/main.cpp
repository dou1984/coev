/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <coev.h>

using namespace coev;

int main()
{
	const int hp = 16 + 4;
	Mempool<128 - hp, 2048 - hp, 4096 - hp> mp;
	auto p = mp.create(100);

	auto _buf = __inner::cast(p);
	mp.destroy(_buf);
	return 0;
}