#pragma once
#include "../coev.h"
#include "loop.h"

namespace coev
{
	class comutex final : public EVRecv
	{
	public:
		awaiter lock();		
		awaiter unlock();		

	private:
		const int on = 1;
		const int off = 0;
		std::recursive_mutex m_lock;
		int m_flag = 0;
	};

}