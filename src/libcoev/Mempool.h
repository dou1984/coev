/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <array>
#include "Chain.h"

namespace coev
{
	namespace __inner
	{
		struct Buffer : Chain
		{
			int m_size = 0;
			int m_verify = 0xabcdef;
			char m_data[0];
			Buffer() = default;
			Buffer(int _size);
		};
		void *alloc(size_t _size);
		void *alloc(Buffer &o, size_t _size);
		void release(Buffer &o, Buffer *_ptr);
		void release(Buffer *_ptr);
		void clear(Buffer &o);
		Buffer *cast(void *_ptr);
	}
	template <size_t... _Size>
	class Mempool;

	template <size_t _Size>
	class Mempool<_Size>
	{
		__inner::Buffer m_data;

	public:
		virtual ~Mempool() { __inner::clear(m_data); }
		void *alloc(size_t s)
		{
			if (s <= _Size)
				return __inner::alloc(m_data, _Size);
			return __inner::alloc(s);
		}
		void release(__inner::Buffer *_buf)
		{
			if (_buf->m_size <= _Size)
				return __inner::release(m_data, _buf);
			__inner::release(_buf);
		}
	};
	template <size_t _Size, size_t... _Res>
	class Mempool<_Size, _Res...> : public Mempool<_Res...>
	{
		__inner::Buffer m_data;

	public:
		~Mempool() { __inner::clear(m_data); }
		void *alloc(size_t s)
		{
			if (s <= _Size)
				return __inner::alloc(_Size);
			return Mempool<_Res...>::alloc(s);
		}
		void release(__inner::Buffer *_buf)
		{
			if (_buf->m_size <= _Size)
				return __inner::release(m_data, _buf);
			return Mempool<_Res...>::release(_buf);
		}
	};
}
