/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *
 */
#include <unordered_map>
#include <mutex>
#include "co_deliver.h"
#include "cosys.h"
#include "local_resume.h"

namespace coev
{

	static std::unordered_map<uint64_t, co_deliver *> all_delivers;
	static std::mutex g_mutex;

	void co_deliver::cb_async(struct ev_loop *loop, ev_async *w, int revents)
	{
		if (revents & EV_ASYNC)
		{
			co_deliver *_this = (co_deliver *)w->data;
			assert(_this != nullptr);
			_this->__resume_ev();
			local_resume();
		}
	}
	co_deliver::co_deliver()
	{
		__init();
		__init_local();
		LOG_CORE("co_deliver::co_deliver %ld\n", gtid());
	}
	co_deliver::~co_deliver()
	{
		__fini();
		__fini_local();
	}
	void co_deliver::__init()
	{
		m_deliver.data = this;
		ev_async_init(&m_deliver, co_deliver::cb_async);
		m_tid = gtid();
		m_loop = cosys::data();
		ev_async_start(m_loop, &m_deliver);
	}
	void co_deliver::__fini()
	{
		if (m_loop)
		{
			ev_async_stop(m_loop, &m_deliver);
			m_loop = nullptr;
		}
	}
	int co_deliver::__done()
	{
		if (m_loop)
		{
			ev_async_send(m_loop, &m_deliver);
		}
		return 0;
	}
	void co_deliver::__init_local()
	{
		std::lock_guard<std::mutex> _(g_mutex);
		all_delivers.emplace(m_tid, this);
	}
	void co_deliver::__fini_local()
	{
		std::lock_guard<std::mutex> _(g_mutex);
		all_delivers.erase(m_tid);
	}

	int co_deliver::call_resume(co_event *ev)
	{
		std::lock_guard<std::mutex> _(m_lock);
		m_waiter.push_back(ev);
		return __done();
	}

	int co_deliver::__resume_ev()
	{
		async _waiter;
		{
			std::lock_guard<std::mutex> _(m_lock);
			m_waiter.move_to(&_waiter);
		}
		_waiter.resume_all();
		return 0;
	}
	bool co_deliver::resume(async &waiter)
	{
		auto c = waiter.pop_front();
		if (c == nullptr)
		{
			return false;
		}
		auto ev = static_cast<co_event *>(c);
		return resume(ev);
	}
	bool co_deliver::resume(co_event *ev)
	{
		std::lock_guard<std::mutex> _(g_mutex);
		if (auto it = all_delivers.find(ev->id()); it != all_delivers.end())
		{
			it->second->call_resume(ev);
			return true;
		}
		return false;
	}
	void co_deliver::stop()
	{
		__fini();
		__fini_local();
	}
	void co_deliver::stop(int tid)
	{
		co_deliver *_this = [=](int tid)
		{
			std::lock_guard<std::mutex> _(g_mutex);
			auto it = all_delivers.find(tid); 			
			return it != all_delivers.end() ? it->second : nullptr;			
		}(tid);
		if (_this)
		{
			_this->stop();
		}
	}
}