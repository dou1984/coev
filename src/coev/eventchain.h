/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <mutex>
#include "chain.h"
#include "object.h"
#include "awaiter.h"
#include "event.h"

namespace coev
{
	template <class TYPE = RECV, class MUTEX = FMUTEX>
	struct eventchain : chain, MUTEX, TYPE
	{
		template <class SUSPEND, class CALL>
		awaiter wait_for(const SUSPEND &suppend, const CALL &call)
		{
			MUTEX::lock();
			if (suppend())
			{
				event ev(this);
				MUTEX::unlock();
				co_await ev;
				MUTEX::lock();
			}
			call();
			MUTEX::unlock();
			co_return 0;
		}
		template <class CALL>
		bool resume(const CALL &call)
		{
			MUTEX::lock();
			auto c = static_cast<event *>(chain::pop_front());
			call();
			MUTEX::unlock();
			if (c)
			{
				c->resume();
				return true;
			}
			return false;
		}
	};

	using EVRecv = eventchain<RECV>;
	using EVSend = eventchain<SEND>;
	using EVEvent = eventchain<EVENT>;
	using EVTask = eventchain<TASK>;
	using EVTimer = eventchain<TIMER>;
	using EVMutex = eventchain<RECV, std::mutex>;

}