/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "taskevent.h"
#include "task.h"
#include "waitfor.h"
namespace coev
{
	void taskevent::__resume()
	{
		if (m_taskchain)
		{
			LOG_CORE("m_taskchain %p\n", m_taskchain);
			auto _taskchain = reinterpret_cast<task *>(m_taskchain);
			_taskchain->__erase(this);
			ts::resume(_taskchain->m_async, []() {});
			m_taskchain = nullptr;
		}
	}
	taskevent::~taskevent()
	{
		__resume();
	}
}