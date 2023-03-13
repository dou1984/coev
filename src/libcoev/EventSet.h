/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include "Chain.h"
#include "Object.h"
#include "Event.h"

namespace coev
{
	template <class TYPE>
	struct EventSet : Chain, TYPE
	{
		bool resume_ex()
		{
			if (Chain::empty())
				return false;
			TRACE();
			auto c = static_cast<Event *>(Chain::pop_front());
			c->resume_ex();
			return true;
		}
	};

	using EVRecv = EventSet<RECV>;
	using EVSend = EventSet<SEND>;
	using EVConnect = EventSet<CONNECT>;
	using EVChannel = EventSet<CHANNEL>;
	using EVEvent = EventSet<EVENT>;
	using EVTask = EventSet<TASK>;
	using EVTimer = EventSet<TIMER>;
	using EVMutex = EventSet<MUTEX>;
}