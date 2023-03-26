/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "loop.h"
#include "Routine.h"

namespace coev
{
	void Routine::__add(const std::function<void()> &f)
	{
		m_list.emplace_back(
			[=]()
			{
				f();
				loop::start();
			});
	}
	void Routine::join()
	{
		for (auto &it : m_list)
			it.join();
	}
}