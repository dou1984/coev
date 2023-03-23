#pragma once

extern "C"
{
	typedef void *(*t_malloc)(size_t size);
	typedef void (*t_free)(void *ptr);	

}