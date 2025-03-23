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
#include "singleton.h"
#include "cosys.h"
#include "local.h"
#include "co_task.h"

namespace coev
{
	class runnable final : public singleton<runnable>
	{
		using func = std::function<awaitable<void>()>;
		using ifunc = std::function<awaitable<int>()>;
		using lfunc = std::function<awaitable<long>()>;
		std::list<std::thread> m_list;

		template <class _F>
		void __add(const _F &_f)
		{
			m_list.emplace_back(
				[=]()
				{
					co_start _f();
					cosys::start();
				});
		}

	public:
		runnable();
		template <class _F>
		runnable &add(const _F &_f)
		{
			__add(_f);
			return *this;
		}
		template <class _F>
		runnable &add(int count, const _F &_f)
		{
			for (int i = 0; i < count; i++)
			{
				__add(_f);
			}
			return *this;
		}

		void join();
		void detach();
	};

}