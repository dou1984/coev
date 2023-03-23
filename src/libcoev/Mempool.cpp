#include "Mempool.h"
#include "Hook.h"

extern t_malloc __real_malloc;
extern t_free __real_free;
namespace coev::__inner
{
	Buffer::Buffer(int _size) : m_size(_size)
	{
	}
	void *create(size_t _size)
	{
		auto _buf = (Buffer *)__real_malloc(sizeof(Buffer) + _size);
		new (_buf) Buffer(_size);
		return _buf->m_data;
	}
	void *create(Buffer &o, size_t _size)
	{
		if (o.empty())
		{
			return create(_size);
		}
		auto _buf = static_cast<Buffer *>(o.pop_front());
		o.m_size--;
		return _buf->m_data;
	}
	void destroy(Buffer &o, Buffer *_buf)
	{
		assert(_buf->m_verify == 0xabcdef);
		o.push_back(_buf);
		o.m_size++;
	}
	void destroy(Buffer *_buf)
	{
		assert(_buf->m_verify == 0xabcdef);
		__real_free(_buf);
	}
	void clear(Buffer &o)
	{
		while (!o.empty())
		{
			destroy(static_cast<Buffer *>(o.pop_front()));
		}
	}
	Buffer *cast(void *_ptr)
	{
		const auto _shift = ((size_t) & (((Buffer *)0)->m_data));
		return (Buffer *)((char *)_ptr - _shift);
	}
}