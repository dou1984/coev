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
#include "local_resume.h"
#include "socket.h"

namespace coev
{
	co_task::~co_task()
	{
		destroy();
	}
	bool co_task::empty()
	{
		std::lock_guard<std::mutex> _(m_waiter.lock());
		return m_promises.empty();
	}
	void co_task::destroy()
	{
		auto _promises = [this]()
		{
			std::lock_guard<std::mutex> _(m_waiter.lock());
			return std::move(m_promises);
		}();
		for (auto it : _promises)
		{
			auto p = it.first;
			assert(p->m_caller == nullptr);
			p->m_task = nullptr;
			std::lock_guard<is_destroying> _(local<is_destroying>::instance());
			p->m_this.destroy();
		}
	}
	awaitable<uint64_t> co_task::wait()
	{
		auto id = co_await m_waiter.suspend(
			[this]() -> bool
			{ return m_promises.size() > 0; },
			[]() {});
		co_return id;
	}
	awaitable<void> co_task::wait_all()
	{
		while (!empty())
		{
			co_await m_waiter.suspend(
				[this]() -> bool
				{ return m_promises.size() > 0; },
				[]() {});
		}
	}
	int co_task::operator<<(promise *_promise)
	{
		return load(_promise);
	}
	int co_task::load(promise *_promise)
	{
		auto __insert = [this](promise *_promise) -> int
		{
			assert(_promise->m_tid == gtid());
			std::lock_guard<std::mutex> _(m_waiter.lock());
			_promise->m_task = this;
			auto id = ++m_id;
			m_promises.emplace(_promise, id);
			return id;
		};
		auto __finally = [](promise *_promise)
		{
			if (_promise->m_status == CORO_SUSPEND)
			{
				assert(_promise->m_status != CORO_FINISHED);
				_promise->m_status = CORO_RUNNING;
				assert(!_promise->m_this.done());
				_promise->m_this.resume();
			}
		};
		auto id = __insert(_promise);
		__finally(_promise);
		return id;
	}

	int co_task::operator>>(promise *_promise)
	{
		return unload(_promise);
	}
	int co_task::unload(promise *_promise)
	{
		auto id = [this, _promise]() -> uint64_t
		{
			std::lock_guard<std::mutex> _(m_waiter.lock());
			if (auto it = m_promises.find(_promise); it != m_promises.end())
			{
				auto id = it->second;
				m_promises.erase(it);
				LOG_CORE("co_task: done gtid:%ld index:%ld\n", _promise->m_tid, id);
				return id;
			}
			return 0;
		}();
		if (id != 0)
		{
			m_waiter.deliver(id);
		}
		return id;
	}
}