/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include "IOContext.h"
#include "Awaiter.h"
#include "Server.h"
#include "Client.h"
#include "Timer.h"
#include "Async.h"
#include "OSSignal.h"
#include "Task.h"
#include "Mutex.h"
#include "Client.h"

namespace coev
{
	
	Awaiter<int> accept(Server &, ipaddress &);
	Awaiter<int> connect(Client &, const char *, int);
	Awaiter<int> send(IOContext &, const char *, int);
	Awaiter<int> recv(IOContext &, char *, int);
	Awaiter<int> recvfrom(IOContext &, char *, int, ipaddress &);
	Awaiter<int> sendto(IOContext &, const char *, int, ipaddress &);
	Awaiter<int> close(IOContext &);
	Awaiter<int> sleep_for(long);
	Awaiter<int> usleep_for(long);

	template <class EV, class OBJ>
	Event wait_for(OBJ &obj)
	{
		EV *ev = &obj;
		return Event{ev};
	}
	template <class... T>
	Awaiter<int> wait_for_any(T &&..._task)
	{
		TaskSet w;
		(w.insert_task(&_task), ...);
		co_await wait_for<EVEvent>(w);
		w.destroy();
		co_return 0;
	}
	template <class... T>
	Awaiter<int> wait_for_all(T &&..._task)
	{
		TaskSet w;
		(w.insert_task(&_task), ...);
		while (w)
			co_await wait_for<EVEvent>(w);
		co_return 0;
	}

}