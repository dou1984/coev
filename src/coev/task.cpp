/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "event.h"
#include "task.h"
#include "awaiter.h"

namespace coev
{
	task::~task()
	{
		destroy();
	}
	void task::insert_task(taskevent *_task)
	{
		auto& _evl = std::get<1>(*this);
		std::lock_guard<std::mutex> _(_evl);
		_evl.push_back(_task);
		_task->m_taskchain = this;
	}
	void task::erase_task(taskevent *_task)
	{
		auto& _evl = std::get<1>(*this);
		std::lock_guard<std::mutex> _(_evl);
		_task->m_taskchain = nullptr;
		_evl.erase(_task);
	}
	void task::destroy()
	{
		auto& _evl = std::get<1>(*this);
		std::lock_guard<std::mutex> _(_evl);
		while (!_evl.empty())
		{
			auto t = static_cast<taskevent *>(_evl.pop_front());
			assert(t->m_taskchain);
			LOG_CORE("t:%p taskchain:%p\n", t, t->m_taskchain);
			t->m_taskchain = nullptr;
			auto a = static_cast<awaiter *>(t);
			a->destroy();
		}
	}
	bool task::empty()
	{
		auto& _evl = std::get<1>(*this);
		std::lock_guard<std::mutex> _(_evl);
		return _evl.empty();
	}

}