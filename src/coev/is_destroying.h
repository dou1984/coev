/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

namespace coev
{
	struct is_destroying
	{
		int m_status = 0;
		operator bool() const;
		void lock();
		void unlock();
	};
}