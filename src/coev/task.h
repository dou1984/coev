/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <memory>
#include <functional>
#include "chain.h"
#include "async.h"
#include "event.h"
#include "tasknotify.h"

namespace coev
{
	class task final
	{		
		async m_listener;
		ts::async m_async;		
		
		void __erase(tasknotify* _notify);
	public:
		virtual ~task();
		void destroy();
		void notify(tasknotify* _notify);
		bool empty();
		event wait_for();
		void insert(tasknotify* _notify);
	};
}
