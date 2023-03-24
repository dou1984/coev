#include <dlfcn.h>
#include "Hook.h"
#include "ThreadLocal.h"
#include "Mempool.h"

using namespace coev;
const int _shift = sizeof(__inner::Buffer);
using mpool = Mempool<0x100 - _shift, 0x400 - _shift, 0x1000 - _shift>;
using tlmp = ThreadLocal<mpool>;

extern "C"
{
	t_malloc __real_malloc;
	t_free __real_free;
	int init_hook()
	{
		__real_malloc = (t_malloc)dlsym(RTLD_NEXT, "malloc");
		__real_free = (t_free)dlsym(RTLD_NEXT, "free");
		return 0;
	}
	/*
	void *malloc(size_t size)
	{
		static auto _init = init_hook();
		return tlmp::instance().alloc(size);
	}
	void free(void *ptr)
	{
		auto _buf = __inner::cast(ptr);
		tlmp::instance().release(_buf);
	}
	*/
}