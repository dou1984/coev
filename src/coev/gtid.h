/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once

namespace coev
{
	uint64_t gtid();

	class ltid
	{
		uint64_t m_tid = 0;
		bool m_set = false;

	public:
		uint64_t id();
		void set(uint64_t id);
		void clear();
		operator bool() const;
	};
}