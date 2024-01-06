#pragma once
#include <mutex>
#include "eventchainmutex.h"
#include "awaiter.h"

namespace coev
{
	class comutex final : public EVMutex
	{
		int m_flag = 0;

	public:
		/*
		awaiter lock();
		awaiter unlock();
		*/
		awaiter lock();
		awaiter unlock();
	};
}