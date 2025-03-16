/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "co_event.h"
#include "co_task.h"
#include "awaitable.h"
#include "wait_for.h"

namespace coev
{
	co_task::~co_task()
	{
		destroy();
	}
	bool co_task::empty()
	{
		std::lock_guard<std::mutex> _(m_task_waiter.m_mutex);
		return m_count == 0;
	}
	void co_task::destroy()
	{
		std::vector<promise *> _promises;
		{
			std::lock_guard<std::mutex> _(m_task_waiter.m_mutex);
			_promises = std::move(m_promises);
			m_count = 0;
		}
		assert(m_task_waiter.empty());
		for (int i = 0; i < _promises.size(); i++)
		{
			if (_promises[i])
			{
				auto p = _promises[i];
				assert(p->m_caller == nullptr);
				LOG_CORE("co_task::destroy() %d %p\n", i, p);
				auto _coro = std::coroutine_handle<promise>::from_promise(*p);
				assert(&_coro.promise() == p);
				std::lock_guard<is_destroying> _(local<is_destroying>::instance());
				_coro.destroy();
			}
		}
	}
	awaitable<int> co_task::wait()
	{
		co_await m_task_waiter.suspend(
			[this]() -> bool
			{ return m_count > 0; },
			[]() {});
		co_return m_id;
	}
	int co_task::insert(promise *_promise)
	{
		std::lock_guard<std::mutex> _(m_task_waiter.m_mutex);
		_promise->m_task = this;
		if (m_promises.size() == m_count++)
		{
			auto i = m_promises.size();
			m_promises.push_back(_promise);
			return i;
		}
		for (int i = 0; i < m_promises.size(); i++)
		{
			if (m_promises[i] == nullptr)
			{
				m_promises[i] = _promise;
				return i;
			}
		}
		throw std::runtime_error("co_task: error m_count");
	}

	int co_task::done(promise *_promise)
	{
		m_task_waiter.resume(
			[=, this]()
			{
				for (int i = 0; i < m_promises.size(); i++)
				{
					if (m_promises[i] == _promise)
					{
						m_promises[i] = nullptr;
						m_count--;
						m_id = i;
						return;
					}
				}
			});
		return m_id;
	}
}