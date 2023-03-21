/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <memory.h>
#include "EventChain.h"

#define SIGNALMIN (60)
#define SIGNALMAX (64)
namespace coev
{
	void ingore_signal(int sign);
	struct OSSignal final : EVRecv
	{	
		OSSignal(uint32_t id);
		virtual ~OSSignal();
		int resume();
		int max() const { return SIGNALMAX; }
		int min() const { return SIGNALMIN; }

		uint32_t m_id = 0;
		uint32_t m_tag = 0;
		struct ev_signal m_Signal;
		static void cb_signal(struct ev_loop *loop, struct ev_signal *w, int revents);
	};
}
