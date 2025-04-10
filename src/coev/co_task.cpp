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
#include "defer.h"
#include "local_resume.h"

namespace coev
{
	co_task::~co_task()
	{
		destroy();
	}
	bool co_task::empty()
	{
		std::lock_guard<std::mutex> _(m_task_waiter.lock());
		return m_count == 0;
	}
	void co_task::destroy()
	{
		std::vector<promise *> _promises;
		{
			std::lock_guard<std::mutex> _(m_task_waiter.lock());
			_promises = std::move(m_promises);
			m_count = 0;
		}
		for (int i = 0; i < _promises.size(); i++)
		{
			if (_promises[i])
			{
				auto p = _promises[i];
				assert(p->m_caller == nullptr);
				LOG_CORE("co_task::destroy() %d %p task:%p\n", i, p, p->m_task);
				auto _coro = std::coroutine_handle<promise>::from_promise(*p);
				assert(&_coro.promise() == p);
				std::lock_guard<is_destroying> _(local<is_destroying>::instance());
				if (!(_coro.done() || p->m_status == CORO_FINISHED))
				{
					_coro.destroy();
				}
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
	awaitable<void> co_task::wait_all()
	{
		while (!empty())
		{
			co_await wait();
		}
	}
	int co_task::operator<<(promise *_promise)
	{
		auto __insert = [this, _promise]() -> int
		{
			assert(_promise->m_tid == gtid());
			std::lock_guard<std::mutex> _(m_task_waiter.lock());
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
		};
		auto __finally = [=]()
		{
			if (_promise->m_status == CORO_SUSPEND)
			{
				LOG_CORE("co_task tid %ld %d\n", _promise->m_tid, _promise->m_status);
				_promise->m_status = CORO_RUNNING;
				auto coro = std::coroutine_handle<promise>::from_promise(*_promise);
				coro.resume();
			}
		};
		auto id = __insert();
		__finally();
		local_resume();
		return id;
	}

	int co_task::operator>>(promise *_promise)
	{
		m_task_waiter.resume(
			[=, this]()
			{
				for (int i = 0; i < m_promises.size(); i++)
				{
					if (m_promises[i] == _promise)
					{
						LOG_CORE("co_task: done %ld %p\n", _promise->m_tid, _promise);
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