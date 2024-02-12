/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <coev.h>
#include "mempool.h"


extern "C"
{
	typedef void *(*t_malloc)(size_t size);
	typedef void *(*t_realloc)(void *ptr, size_t size);
	typedef void (*t_free)(void *ptr);
	typedef void *(*t_calloc)(size_t nitems, size_t size);

	namespace coev
	{
		using namespace __inner;
		using mpool = mempool<0x100 - sizeof(Buffer), 0x400 - sizeof(Buffer), 0x1000 - sizeof(Buffer)>;
		using tlmp = threadlocal<mpool>;
	}
}