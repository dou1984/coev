/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "co_event.h"
#include "co_task.h"
#include "awaitable.h"
#include "wait_for.h"
#include "local_resume.h"
#include "socket.h"

#define TASK_ID_MAX (1000000000000000000)

namespace coev
{
	void __destroy(promise *_promise)
	{
		if (_promise)
		{
			LOG_CORE("promise tid:%ld %p", _promise->m_tid, _promise->m_this.address());
			_promise->m_task = nullptr;
			_promise->m_type = details::CORO_NONE;
			_promise->m_this.destroy();
		}
	}

	int64_t __increase(int64_t &id, std::set<int64_t> &ids)
	{
		assert(ids.size() < (TASK_ID_MAX / 2));
		while (true)
		{
			++id;
			if (id > TASK_ID_MAX)
			{
				id = 0;
			}
			if (ids.find(id) == ids.end())
			{
				ids.insert(id);
				return id;
			}
		}
	}
	void __finally(promise *_promise)
	{
		if (_promise->m_status == details::CORO_SUSPEND)
		{
			_promise->m_status = details::CORO_RUNNING;
			assert(!_promise->m_this.done());
			_promise->m_this.resume();
		}
	}
	co_task::~co_task()
	{
		destroy();
	}
	bool co_task::empty()
	{
		return m_promises.empty();
	}
	void co_task::destroy()
	{
		auto _promises = std::move(m_promises);
		for (auto it = _promises.begin(); it != _promises.end(); ++it)
		{
			auto p = it->first;
			std::lock_guard<is_destroying> _(local<is_destroying>::instance());
			__destroy(p);
		}
	}
	void co_task::destroy(int64_t id)
	{
		for (auto it = m_promises.begin(); it != m_promises.end(); ++it)
		{
			if (it->second == id)
			{
				auto p = it->first;
				m_promises.erase(it);
				std::lock_guard<is_destroying> _(local<is_destroying>::instance());
				__destroy(p);
				break;
			}
		}
	}
	awaitable<int64_t> co_task::wait()
	{
		if (m_promises.size() > 0)
		{
			co_return co_await m_waiter.suspend();
		}
		co_return 0;
	}
	awaitable<void> co_task::wait_all()
	{
		while (m_promises.size() > 0)
		{
			co_await m_waiter.suspend();
		}
	}
	int co_task::load(promise *_promise)
	{
		assert(_promise != nullptr);
		auto __insert = [this](promise *_promise) -> int
		{
			assert(_promise->m_tid == gtid());
			assert(_promise->m_caller == nullptr);
			_promise->m_task = this;
			_promise->m_type = details::CORO_TASK;
			auto id = __increase(m_id, m_ids);
			m_promises.emplace(_promise, id);
			return id;
		};
		auto id = __insert(_promise);
		__finally(_promise);
		return id;
	}

	int co_task::unload(promise *_promise)
	{
		if (auto it = m_promises.find(_promise); it != m_promises.end())
		{
			auto id = it->second;
			m_promises.erase(it);
			m_ids.erase(id);
			LOG_CORE("co_task: done gtid:%ld index:%ld", _promise->m_tid, id);
			m_waiter.resume(id);
			return id;
		}
		return 0;
	}

	namespace guard
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
			for (auto it = _promises.begin(); it != _promises.end(); ++it)
			{
				auto p = it->first;
				std::lock_guard<is_destroying> _(local<is_destroying>::instance());
				__destroy(p);
			}
		}
		void co_task::destroy(int64_t id)
		{
			promise *p = nullptr;
			{
				std::lock_guard<std::mutex> _(m_waiter.lock());
				for (auto it = m_promises.begin(); it != m_promises.end(); ++it)
				{
					if (it->second == id)
					{
						p = it->first;
						m_promises.erase(it);
						break;
					}
				}
			}
			std::lock_guard<is_destroying> _(local<is_destroying>::instance());
			__destroy(p);
		}
		awaitable<int64_t> co_task::wait()
		{
			co_return co_await m_waiter.suspend(
				[this]() -> bool
				{ return m_promises.size() > 0; });
		}
		awaitable<void> co_task::wait_all()
		{
			while (!empty())
			{
				co_await m_waiter.suspend(
					[this]() -> bool
					{ return m_promises.size() > 0; });
			}
		}
		int co_task::load(promise *_promise)
		{
			assert(_promise != nullptr);
			auto __insert = [this](promise *_promise) -> int
			{
				assert(_promise->m_tid == gtid());
				assert(_promise->m_caller == nullptr);
				std::lock_guard<std::mutex> _(m_waiter.lock());
				_promise->m_task = this;
				_promise->m_type = details::CORO_GUARD_TASK;
				auto id = __increase(m_id, m_ids);
				m_promises.emplace(_promise, id);
				return id;
			};
			auto id = __insert(_promise);
			__finally(_promise);
			return id;
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
					m_ids.erase(id);
					LOG_CORE("co_task: done gtid:%ld index:%ld", _promise->m_tid, id);
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

}