/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2022, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once

namespace coev
{
	struct Object
	{
	};
	struct AWAITER : Object
	{
		void resume_ex()
		{
		}
	};
	struct CONNECT
	{
	};
	struct RECV
	{
	};
	struct SEND
	{
	};
	struct CLOSE
	{
	};
	struct EVENT
	{
	};
	struct TASK
	{
	};
	struct CHANNEL
	{
	};
	struct ASYNC
	{
	};
	struct SIGNAL
	{
	};
	struct ACCEPT
	{
	};
	struct TIMER
	{
	};
	struct MUTEX
	{
	};
	struct ASYNCEVENT
	{
	};	
}