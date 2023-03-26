/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <memory>
#include "iocontext.h"
#include "awaiter.h"
#include "Server.h"
#include "Client.h"
#include "Timer.h"
#include "async.h"
#include "OSSignal.h"
#include "task.h"
#include "Mutex.h"

namespace coev
{
	using SharedIO = std::shared_ptr<iocontext>;
	awaiter<SharedIO> accept(Server &, ipaddress &);
	awaiter<int> connect(client &, const char *, int);
	awaiter<int> send(iocontext &, const char *, int);
	awaiter<int> recv(iocontext &, char *, int);
	awaiter<int> recvfrom(iocontext &, char *, int, ipaddress &);
	awaiter<int> sendto(iocontext &, const char *, int, ipaddress &);
	awaiter<int> close(iocontext &);
	awaiter<int> sleep_for(long);
	awaiter<int> usleep_for(long);

	template <class EV, class OBJ>
	event wait_for(OBJ &obj)
	{
		EV *ev = &obj;
		return event{ev};
	}
	template <class... T>
	awaiter<int> wait_for_any(T &&..._task)
	{
		taskchain w;
		(w.insert_task(&_task), ...);
		co_await wait_for<EVEvent>(w);
		w.destroy();
		co_return 0;
	}
	template <class... T>
	awaiter<int> wait_for_all(T &&..._task)
	{
		taskchain w;
		(w.insert_task(&_task), ...);
		while (w)
			co_await wait_for<EVEvent>(w);
		co_return 0;
	}

}