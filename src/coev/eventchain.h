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
#include "event.h"

namespace coev
{
	template <class TYPE>
	struct eventchain : chain,  TYPE
	{		
		event wait_for()
		{
			return event(this);
		}
		bool resume()
		{
			auto c = static_cast<event *>(chain::pop_front());
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
}