#include "Mempool.h"
#include "Hook.h"

extern t_malloc __real_malloc;
extern t_free __real_free;
namespace coev::__inner
{
	Buffer::Buffer(int _size) : m_size(_size)
	{
	}
	void *alloc(size_t _size)
	{
		auto _buf = new (__real_malloc(sizeof(Buffer) + _size)) Buffer(_size);
		return _buf->m_data;
	}
	void *alloc(Buffer &o, size_t _size)
	{
		if (o.empty())
		{
			return alloc(_size);
		}
		auto _buf = static_cast<Buffer *>(o.pop_front());
		assert(_buf->m_verify == MAGICWORD);
		o.m_size--;
		return _buf->m_data;
	}
	void release(Buffer &o, Buffer *_buf)
	{
		assert(_buf->m_verify == MAGICWORD);
		o.push_back(_buf);
		o.m_size++;
	}
	void release(Buffer *_buf)
	{
		assert(_buf->m_verify == MAGICWORD);
		__real_free(_buf);
	}
	void clear(Buffer &o)
	{
		while (!o.empty())
		{
			release(static_cast<Buffer *>(o.pop_front()));
		}
	}
	Buffer *cast(void *_ptr)
	{
		const auto _shift = ((size_t) & (((Buffer *)0)->m_data));
		return (Buffer *)((char *)_ptr - _shift);
	}
}