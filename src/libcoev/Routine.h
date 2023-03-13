#pragma once
#include <thread>
#include <list>
#include <functional>
#include "Loop.h"

namespace coev
{
	class Routine 
	{
		std::list<std::thread> m_list;
		void __add(const std::function<void()> &);

	public:
		template <class _F>
		void add(const _F &_f)
		{
			__add([=]()
				  { _f(); });
		}
		void join();
	};
}