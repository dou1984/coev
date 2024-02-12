/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <string.h>
#include <dlfcn.h>
#include <coev.h>
#include "hook.h"
#include "mempool.h"


using namespace coev;

extern "C"
{
#ifdef __MEMPOOL__
	bool enable_mempool = true;
	t_malloc __real_malloc;
	t_free __real_free;
	t_realloc __real_realloc;
	t_calloc __real_calloc;
	int __init_hook()
	{
		__real_malloc = (t_malloc)dlsym(RTLD_NEXT, "malloc");
		__real_realloc = (t_realloc)dlsym(RTLD_NEXT, "realloc");
		__real_free = (t_free)dlsym(RTLD_NEXT, "free");
		__real_calloc = (t_calloc)dlsym(RTLD_NEXT, "calloc");
		return 0;
	}
	int init_hook()
	{
		static auto r = __init_hook();
		return r;
	}
	void *malloc(size_t size)
	{
		static auto _init = init_hook();
		if (!enable_mempool)
		{
			return __real_malloc(size);
		}
		return tlmp::instance().alloc(size);
	}
	void *realloc(void *ptr, size_t size)
	{
		static auto _init = init_hook();
		if (!enable_mempool)
		{
			return __real_realloc(ptr, size);
		}
		auto _buf = __inner::cast(ptr);
		if (size <= _buf->m_size)
		{
			return _buf->m_data;
		}
		free(ptr);
		return malloc(size);
	}
	void *calloc(size_t nitems, size_t size)
	{
		static auto _init = init_hook();
		if (!enable_mempool)
		{
			return __real_calloc(nitems, size);
		}
		int total = nitems * size;
		auto ptr = tlmp::instance().alloc(total);
		memset(ptr, 0, total);
		return ptr;
	}
	void free(void *ptr)
	{
		static auto _init = init_hook();
		if (!enable_mempool)
		{
			__real_free(ptr);
		}
		else
		{
			auto _buf = __inner::cast(ptr);
			tlmp::instance().release(_buf);
		}
	}
#endif
}