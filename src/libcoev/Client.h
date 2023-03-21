/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <ev.h>
#include "Socket.h"
#include "Event.h"
#include "EventChain.h"
#include "IOContext.h"

namespace coev
{	
	struct Client : IOContext
	{		
		Client();
		virtual ~Client();
		int connect(const char *ip, int port);	
		int connect_insert();
		int connect_remove();	
		static void cb_connect(struct ev_loop *loop, struct ev_io *w, int revents);
	};

}
