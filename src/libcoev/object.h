/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
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
	struct RECV
	{
	};
	struct SEND
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
	struct TIMER
	{
	};
	struct MUTEX
	{
	};	
}