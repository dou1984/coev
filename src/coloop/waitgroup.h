#pragma once
#include "../coev.h"
#include "loop.h"
namespace coev
{
	class waitgroup final : public EVRecv
	{
	public:
		int add(int c = 1);
		int done();
		event wait();

	private:
		std::recursive_mutex m_lock;
		int m_count = 0;
	};
}