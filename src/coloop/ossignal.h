/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <memory.h>
#include "../coev.h"

#define SIGNALMIN (60)
#define SIGNALMAX (64)
namespace coev
{
	struct ossignal final : EVRecv
	{
		ossignal(uint64_t id);
		virtual ~ossignal();
		int resume();
		int max() const { return SIGNALMAX; }
		int min() const { return SIGNALMIN; }

		uint64_t m_id = 0;
		uint64_t m_tag = 0;
		struct ev_signal m_Signal;
		static void cb_signal(struct ev_loop *loop, struct ev_signal *w, int revents);
	};
}
