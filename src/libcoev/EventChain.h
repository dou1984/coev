/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
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
	struct EventChain : Chain, TYPE
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

	using EVRecv = EventChain<RECV>;
	using EVSend = EventChain<SEND>;
	using EVChannel = EventChain<CHANNEL>;
	using EVEvent = EventChain<EVENT>;
	using EVTask = EventChain<TASK>;
	using EVTimer = EventChain<TIMER>;
	using EVMutex = EventChain<MUTEX>;
}