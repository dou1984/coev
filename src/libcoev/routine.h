/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <thread>
#include <list>
#include <functional>
#include "loop.h"
#include "singleton.h"

namespace coev
{
	class routine final : public singleton<routine>
	{
		std::list<std::thread> m_list;
		void __add(const std::function<void()> &);

	public:
		routine();
		template <class _F>
		routine &add(const _F &_f)
		{
			__add([=]()
				  { _f(); });
			return *this;
		}
		template <class _F>
		routine &add(int count, const _F &_f)
		{
			for (int i = 0; i < count; i++)
				__add([=]()
					  { _f(); });
			return *this;
		}
		void join();
	};

}